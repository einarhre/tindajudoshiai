/* -*- mode: C; c-basic-offset: 4;  -*- */

/*
 * Copyright (C) 2006-2010 by Hannu Jokinen
 * Full copyright text is included in the software package.
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <assert.h>

#include "sqlite3.h"
#include "judoshiai.h"

static gint category_system;
static struct judoka j;

static int db_callback_categories(void *data, int argc, char **argv, char **azColName)
{
    gint i, flags = (int)data;

    for (i = 0; i < argc; i++) {
        //g_print(" %s=%s", azColName[i], argv[i]);
        if (IS(index))
            j.index = atoi(argv[i]);
        else if (IS(category))
            j.last = argv[i];
        else if (IS(tatami))
            j.belt = atoi(argv[i]);
        else if (IS(deleted))
            j.deleted = atoi(argv[i]);
        else if (IS(group))
            j.birthyear = atoi(argv[i]);
        else if (IS(system))
            category_system = atoi(argv[i]);
    }
    //g_print("\n");

    j.visible = FALSE;

    if (j.index >= current_category_index)
        current_category_index = j.index + 1;

    if (j.deleted & DELETED)
        return 0;

    if (flags & DB_GET_SYSTEM)
        return 0;

    display_one_judoka(&j);

    avl_set_category(j.index, j.last, j.belt, j.birthyear, category_system);

    return 0;
}

void db_add_category(int num, struct judoka *j)
{
    char buffer[200];

    if (num < 10000) {
        g_print("%s: ERROR, num = %d\n", __FUNCTION__, num);
        return;
    }

    sprintf(buffer, 
            "INSERT INTO categories VALUES ("
            "%d, \"%s\", %d, %d, %d, %d "
            ")", 
            num, j->last, j->belt, j->deleted, j->birthyear, 0/* system */);

    db_exec(db_name, buffer, NULL, db_callback_categories);

    avl_set_category(num, j->last, j->belt, j->birthyear, 0);
}

void db_update_category(int num, struct judoka *j)
{
    if (num < 10000) {
        g_print("%s: ERROR, num = %d\n", __FUNCTION__, num);
        return;
    }

    struct judoka *old = get_data(num);
    if (old) {
        db_exec_str(NULL, NULL,
                    "UPDATE competitors SET \"category\"=\"%s\" "
                    "WHERE \"category\"=\"%s\"", 
                    j->last, old->last);

        // tatami changed. remove comments to avoid conflicts.
        if (old->belt != j->belt) {
            db_exec_str(NULL, NULL, 
                        "UPDATE matches SET \"comment\"=0 "
                        "WHERE \"category\"=%d AND "
                        "(\"comment\"=%d OR \"comment\"=%d) ",
                        num, COMMENT_MATCH_1, COMMENT_MATCH_2);
        }

        free_judoka(old);
    }

    db_exec_str(NULL, db_callback_categories, 
		"UPDATE categories SET "
		"\"category\"=\"%s\", \"tatami\"=%d, \"deleted\"=%d, \"group\"=%d "
		"WHERE \"index\"=%d",
		j->last, j->belt, j->deleted, j->birthyear, num);

    struct category_data data, *data1;
    data.index = num;
    if (avl_get_by_key(categories_tree, &data, (void *)&data1) == 0) {
        strncpy(data1->category, j->last, sizeof(data1->category)-1);
        data1->tatami = j->belt;
        data1->group = j->birthyear;
        set_category_to_queue(data1);
    } else
        g_print("Error %s %d (cat %d)\n", __FUNCTION__, __LINE__, num);
}

void db_set_system(int num, int sys)
{
    char buffer[1000];

    sprintf(buffer, 
            "UPDATE categories SET "
            "\"system\"=%d WHERE \"index\"=%d",
            sys, num);

    db_exec(db_name, buffer, NULL, db_callback_categories);

    struct category_data data, *data1;
    data.index = num;
    if (avl_get_by_key(categories_tree, &data, (void *)&data1) == 0) {
        data1->system = sys;
        set_category_to_queue(data1);
    } else
        g_print("Error %s %d (num=%d sys=%d)\n", __FUNCTION__, __LINE__,
                num, sys);
}

gint db_get_system(gint num)
{
    char buffer[1000];

    category_system = 0;
    sprintf(buffer, "SELECT * FROM categories WHERE \"index\"=%d", num);

    db_exec(db_name, buffer, (gpointer)DB_GET_SYSTEM, db_callback_categories);

    return category_system;
}

gint db_get_tatami(gint num)
{
    char buffer[128];

    sprintf(buffer, "SELECT * FROM categories WHERE \"index\"=%d", num);

    db_exec(db_name, buffer, (gpointer)DB_GET_SYSTEM, db_callback_categories);

    return j.belt;
}

void db_read_categories(void)
{
    char buffer[100];

    sprintf(buffer, 
            "SELECT * FROM categories");
    db_exec(db_name, buffer, NULL, db_callback_categories);
}
