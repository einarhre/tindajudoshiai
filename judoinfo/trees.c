/* -*- mode: C; c-basic-offset: 4;  -*- */

/*
 * Copyright 2006-2023 Hannu Jokinen
 * 
 * This file is part of JudoShiai.
 *
 * JudoShiai is free software: you can redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * JudoShiai is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with JudoShiai. If not, see
 * <https://www.gnu.org/licenses/>.
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef EMSCRIPTEN
#include <gtk/gtk.h>
#endif
#include <assert.h>

#include "avl.h"
#include "judoinfo.h"

avl_tree *competitors_tree = NULL;

static int competitors_avl_compare(void *compare_arg, void *a, void *b)
{
    struct name_data *a1 = a;
    struct name_data *b1 = b;
    return a1->index - b1->index;
}

static int free_avl_key(void *key)
{
    struct name_data *x = key;
    g_free(x->last);
    g_free(x->first);
    g_free(x->club);
    g_free(key);
    return 1;
}

struct name_data *avl_get_data(gint index)
{
    struct name_data data;
    void *data1;
    index &= 0xff00ffff;
    
    data.index = index;
    if (avl_get_by_key(competitors_tree, &data, &data1) == 0) {
        return data1;
    }

    // request data
    ask_for_data(index);

    return NULL;
}

void avl_set_data(gint index, gchar *first, gchar *last, gchar *club)
{
    struct name_data *data;
    void *data1;
    index &= 0xff00ffff;

    data = g_malloc(sizeof(*data));
    memset(data, 0, sizeof(*data));
	
    data->index = index;
    data->last = g_strdup(last);
    data->first = g_strdup(first);
    data->club = g_strdup(club);

    if (avl_get_by_key(competitors_tree, data, &data1) == 0) {
        /* data exists */
        avl_delete(competitors_tree, data, free_avl_key);
    }
    avl_insert(competitors_tree, data);
}

void init_trees(void)
{
    if (competitors_tree)
        avl_tree_free(competitors_tree, free_avl_key);
    competitors_tree = avl_tree_new(competitors_avl_compare, NULL);

    avl_set_data(0, "", "", "");
    avl_set_data(1000, "", "", "");
}
