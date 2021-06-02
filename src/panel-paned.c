/* panel-paned.c
 *
 * Copyright 2021 Christian Hergert <chergert@redhat.com>
 *
 * This file is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "config.h"

#include <string.h>

#include "panel-dock-private.h"
#include "panel-paned-private.h"
#include "panel-resizer-private.h"

struct _PanelPaned
{
  GtkWidget      parent_instance;
  GtkOrientation orientation;
};

G_DEFINE_TYPE_WITH_CODE (PanelPaned, panel_paned, GTK_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_ORIENTABLE, NULL))

enum {
  PROP_0,
  N_PROPS,

  PROP_ORIENTATION,
};

/**
 * panel_paned_new:
 *
 * Create a new #PanelPaned.
 *
 * Returns: (transfer full): a newly created #PanelPaned
 */
GtkWidget *
panel_paned_new (void)
{
  return g_object_new (PANEL_TYPE_PANED, NULL);
}

static void
panel_paned_set_orientation (PanelPaned     *self,
                             GtkOrientation  orientation)
{
  PanelDockPosition dockpos;

  g_assert (PANEL_IS_PANED (self));
  g_assert (orientation == GTK_ORIENTATION_HORIZONTAL ||
            orientation == GTK_ORIENTATION_VERTICAL);

  if (self->orientation == orientation)
    return;

  self->orientation = orientation;

  if (self->orientation == GTK_ORIENTATION_HORIZONTAL)
    dockpos = PANEL_DOCK_POSITION_START;
  else
    dockpos = PANEL_DOCK_POSITION_TOP;

  for (GtkWidget *child = gtk_widget_get_last_child (GTK_WIDGET (self));
       child != NULL;
       child = gtk_widget_get_prev_sibling (child))
    {
      g_assert (PANEL_IS_RESIZER (child));

      panel_resizer_set_position (PANEL_RESIZER (child), dockpos);
    }

  _panel_dock_update_orientation (GTK_WIDGET (self), self->orientation);
  gtk_widget_queue_resize (GTK_WIDGET (self));
  g_object_notify (G_OBJECT (self), "orientation");
}

static void
panel_paned_measure (GtkWidget      *widget,
                     GtkOrientation  orientation,
                     int             for_size,
                     int            *minimum,
                     int            *natural,
                     int            *minimum_baseline,
                     int            *natural_baseline)
{
  PanelPaned *self = (PanelPaned *)widget;

  g_assert (PANEL_IS_PANED (self));

  *minimum = 0;
  *natural = 0;
  *minimum_baseline = -1;
  *natural_baseline = -1;

  for (GtkWidget *child = gtk_widget_get_first_child (widget);
       child != NULL;
       child = gtk_widget_get_next_sibling (child))
    {
      int child_min, child_nat;

      gtk_widget_measure (child, orientation, for_size, &child_min, &child_nat, NULL, NULL);

      if (orientation == self->orientation)
        {
          *minimum += child_min;
          *natural += child_nat;
        }
      else
        {
          *minimum = MAX (*minimum, child_min);
          *natural = MAX (*natural, child_nat);
        }
    }
}

static inline guint
panel_paned_get_n_children (PanelPaned *self)
{
  guint count = 0;
  for (GtkWidget *child = gtk_widget_get_first_child (GTK_WIDGET (self));
       child != NULL;
       child = gtk_widget_get_next_sibling (child))
    count++;
  return count;
}

typedef struct
{
  GtkWidget      *widget;
  GtkRequisition  min_request;
  GtkRequisition  nat_request;
  GtkAllocation   alloc;
} ChildAllocation;

static void
panel_paned_size_allocate (GtkWidget *widget,
                           int        width,
                           int        height,
                           int        baseline)
{
  PanelPaned *self = (PanelPaned *)widget;
  ChildAllocation *allocs;
  GtkOrientation orientation;
  guint n_children = 0;
  guint i;
  int extra_width = width;
  int extra_height = height;
  int x, y;

  g_assert (PANEL_IS_PANED (self));

  GTK_WIDGET_CLASS (panel_paned_parent_class)->size_allocate (widget, width, height, baseline);

  n_children = panel_paned_get_n_children (self);

  if (n_children == 1)
    {
      GtkWidget *child = gtk_widget_get_first_child (widget);
      GtkAllocation alloc = { 0, 0, width, height };

      if (gtk_widget_get_visible (child))
        {
          gtk_widget_size_allocate (child, &alloc, -1);
          return;
        }
    }

  orientation = gtk_orientable_get_orientation (GTK_ORIENTABLE (self));
  allocs = g_newa (ChildAllocation, n_children);
  memset (allocs, 0, sizeof *allocs * n_children);

  i = 0;
  for (GtkWidget *child = gtk_widget_get_first_child (GTK_WIDGET (self));
       child != NULL;
       child = gtk_widget_get_next_sibling (child), i++)
    {
      ChildAllocation *child_alloc = &allocs[i];

      if (!gtk_widget_get_visible (child))
        continue;

      gtk_widget_measure (child, GTK_ORIENTATION_HORIZONTAL, height,
                          &child_alloc->min_request.width,
                          &child_alloc->nat_request.width,
                          NULL, NULL);
      gtk_widget_measure (child, GTK_ORIENTATION_VERTICAL, width,
                          &child_alloc->min_request.height,
                          &child_alloc->nat_request.height,
                          NULL, NULL);

      child_alloc->alloc.width = child_alloc->min_request.width;
      child_alloc->alloc.height = child_alloc->min_request.height;

      if (orientation == GTK_ORIENTATION_HORIZONTAL)
        {
          extra_width -= child_alloc->alloc.width;
          child_alloc->alloc.height = height;
        }
      else
        {
          extra_height -= child_alloc->alloc.height;
          child_alloc->alloc.width = width;
        }
    }

  i = 0;
  for (GtkWidget *child = gtk_widget_get_first_child (GTK_WIDGET (self));
       child != NULL;
       child = gtk_widget_get_next_sibling (child), i++)
    {
      ChildAllocation *child_alloc = &allocs[i];

      if (extra_width <= 0)
        break;

      if (orientation == GTK_ORIENTATION_HORIZONTAL)
        {
          int taken = MIN (extra_width, child_alloc->nat_request.width - child_alloc->alloc.width);

          if (taken > 0)
            {
              child_alloc->alloc.width += taken;
              extra_width -= taken;
            }
        }
      else
        {
          int taken = MIN (extra_height, child_alloc->nat_request.height - child_alloc->alloc.height);

          if (taken > 0)
            {
              child_alloc->alloc.height += taken;
              extra_height -= taken;
            }
        }
    }

  i = 0;
  x = 0;
  y = 0;
  for (GtkWidget *child = gtk_widget_get_first_child (GTK_WIDGET (self));
       child != NULL;
       child = gtk_widget_get_next_sibling (child), i++)
    {
      ChildAllocation *child_alloc = &allocs[i];

      child_alloc->alloc.x = x;
      child_alloc->alloc.y = y;

      if (orientation == GTK_ORIENTATION_HORIZONTAL)
        x += child_alloc->alloc.width;
      else
        y += child_alloc->alloc.height;

      gtk_widget_size_allocate (child, &child_alloc->alloc, -1);
    }
}

static void
panel_paned_dispose (GObject *object)
{
  PanelPaned *self = (PanelPaned *)object;
  GtkWidget *child;

  child = gtk_widget_get_first_child (GTK_WIDGET (self));
  while (child)
    {
      GtkWidget *next = gtk_widget_get_next_sibling (child);
      gtk_widget_unparent (child);
      child = next;
    }

  G_OBJECT_CLASS (panel_paned_parent_class)->dispose (object);
}

static void
panel_paned_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  PanelPaned *self = PANEL_PANED (object);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, self->orientation);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
panel_paned_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  PanelPaned *self = PANEL_PANED (object);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      panel_paned_set_orientation (self, g_value_get_enum (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
panel_paned_class_init (PanelPanedClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = panel_paned_dispose;
  object_class->get_property = panel_paned_get_property;
  object_class->set_property = panel_paned_set_property;

  widget_class->measure = panel_paned_measure;
  widget_class->size_allocate = panel_paned_size_allocate;

  g_object_class_override_property (object_class, PROP_ORIENTATION, "orientation");

  gtk_widget_class_set_css_name (widget_class, "panelpaned");
}

static void
panel_paned_init (PanelPaned *self)
{
  self->orientation = GTK_ORIENTATION_HORIZONTAL;
}

static void
panel_paned_update_handles (PanelPaned *self)
{
  GtkWidget *child;

  g_assert (PANEL_IS_PANED (self));

  for (child = gtk_widget_get_first_child (GTK_WIDGET (self));
       child != NULL;
       child = gtk_widget_get_next_sibling (child))
    {
      GtkWidget *handle;

      g_assert (PANEL_IS_RESIZER (child));

      if ((handle = panel_resizer_get_handle (PANEL_RESIZER (child))))
        gtk_widget_show (handle);
    }

  if ((child = gtk_widget_get_last_child (GTK_WIDGET (self))))
    {
      GtkWidget *handle = panel_resizer_get_handle (PANEL_RESIZER (child));
      gtk_widget_hide (handle);
    }
}

void
panel_paned_remove (PanelPaned *self,
                    GtkWidget  *child)
{
  GtkWidget *parent;

  g_return_if_fail (PANEL_IS_PANED (self));
  g_return_if_fail (GTK_IS_WIDGET (child));

  parent = gtk_widget_get_parent (child);

  g_return_if_fail (parent != NULL &&
                    gtk_widget_get_parent (parent) == GTK_WIDGET (self));

  gtk_widget_unparent (GTK_WIDGET (parent));

  panel_paned_update_handles (self);
}

void
panel_paned_insert (PanelPaned *self,
                    int         position,
                    GtkWidget  *child)
{
  PanelDockPosition dockpos;
  GtkWidget *resizer;

  g_return_if_fail (PANEL_IS_PANED (self));
  g_return_if_fail (GTK_IS_WIDGET (child));
  g_return_if_fail (gtk_widget_get_parent (child) == NULL);

  if (self->orientation == GTK_ORIENTATION_HORIZONTAL)
    dockpos = PANEL_DOCK_POSITION_START;
  else
    dockpos = PANEL_DOCK_POSITION_TOP;

  resizer = panel_resizer_new (dockpos);
  panel_resizer_set_child (PANEL_RESIZER (resizer), child);

  if (position < 0)
    gtk_widget_insert_before (GTK_WIDGET (resizer), GTK_WIDGET (self), NULL);
  else if (position == 0)
    gtk_widget_insert_after (GTK_WIDGET (resizer), GTK_WIDGET (self), NULL);
  else
    {
      GtkWidget *sibling = gtk_widget_get_first_child (GTK_WIDGET (self));

      for (int i = position; i > 0 && sibling != NULL; i--)
        sibling = gtk_widget_get_next_sibling (sibling);

      gtk_widget_insert_before (GTK_WIDGET (resizer), GTK_WIDGET (self), sibling);
    }

  panel_paned_update_handles (self);

  gtk_widget_queue_resize (GTK_WIDGET (self));
}

void
panel_paned_append (PanelPaned *self,
                    GtkWidget  *child)
{
  panel_paned_insert (self, -1, child);
}

void
panel_paned_prepend (PanelPaned *self,
                     GtkWidget  *child)
{
  panel_paned_insert (self, 0, child);
}
