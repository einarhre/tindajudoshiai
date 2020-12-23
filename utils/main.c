#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#include <windows.h> 
#endif

#include "sqlite3.h"

#define IS(x) (!strcmp(azColName[i], #x))

static int color_exists = 0;
static int db_version = 29;

static int callback_tables(void *data, int argc, char **argv, char **azColName)
{
    int i;

    for (i = 0; i < argc; i++) {
	if (!argv[i]) continue;
        if (IS(sql)) {
            if (strstr(argv[i], "categories") && strstr(argv[i], "color"))
                color_exists = 1;
        }
    }

    return 0;
}

const char *table_competitors_29 =
    "CREATE TABLE competitors (\"index\" INTEGER, \"last\" TEXT, \"first\" TEXT, \"birthyear\" INTEGER, \"belt\" INTEGER, \"club\" TEXT, "
    "\"regcategory\" TEXT, \"weight\" INTEGER, \"visible\" INTEGER, \"category\" TEXT, \"deleted\" INTEGER, \"country\" TEXT, \"id\" TEXT, "
    "\"seeding\" INTEGER, \"clubseeding\" INTEGER, \"comment\" TEXT, \"coachid\" TEXT, UNIQUE(\"index\"))";

const char *table_categories_29 =
    "CREATE TABLE categories (\"index\" INTEGER, \"category\" TEXT, \"tatami\" INTEGER, \"deleted\" INTEGER, \"group\" INTEGER, "
    "\"system\" INTEGER, \"numcomp\" INTEGER, \"table\" INTEGER, \"wishsys\" INTEGER, \"pos1\" INTEGER, \"pos2\" INTEGER, \"pos3\" INTEGER, "
    "\"pos4\" INTEGER, \"pos5\" INTEGER, \"pos6\" INTEGER, \"pos7\" INTEGER, \"pos8\" INTEGER, UNIQUE(\"index\"))";
const char *table_categories_30 =
    "CREATE TABLE categories (\"index\" INTEGER, \"category\" TEXT, \"tatami\" INTEGER, \"deleted\" INTEGER, \"group\" INTEGER, "
    "\"system\" INTEGER, \"numcomp\" INTEGER, \"table\" INTEGER, \"wishsys\" INTEGER, \"pos1\" INTEGER, \"pos2\" INTEGER, \"pos3\" INTEGER, "
    "\"pos4\" INTEGER, \"pos5\" INTEGER, \"pos6\" INTEGER, \"pos7\" INTEGER, \"pos8\" INTEGER, \"color\" TEXT, UNIQUE(\"index\"))";

const char *table_matches_29 =
    "CREATE TABLE matches (\"category\" INTEGER, \"number\" INTEGER,\"blue\" INTEGER, \"white\" INTEGER,\"blue_score\" INTEGER, "
    "\"white_score\" INTEGER, \"blue_points\" INTEGER, \"white_points\" INTEGER, \"time\" INTEGER, \"comment\" INTEGER, \"deleted\" INTEGER, "
    "\"forcedtatami\" INTEGER, \"forcednumber\" INTEGER, \"date\" INTEGER, \"legend\" INTEGER, UNIQUE(\"category\", \"number\"))";
    
const char *table_info_29 =
    "CREATE TABLE \"info\" (\"item\" TEXT, \"value\" TEXT, UNIQUE(\"item\"))";
    
const char *table_catdef_29 =
    "CREATE TABLE \"catdef\" (\"age\" INTEGER, \"agetext\" TEXT, \"flags\" INTEGER, \"weight\" INTEGER, \"weighttext\" TEXT, \"matchtime\" INTEGER, "
    "\"pintimekoka\" INTEGER, \"pintimeyuko\" INTEGER, \"pintimewazaari\" INTEGER, \"pintimeippon\" INTEGER, \"resttime\" INTEGER, "
    "\"gstime\" INTEGER, \"reptime\" INTEGER, \"layout\" TEXT)";

const char *columns_categories_29 = "\"index\", \"category\", \"tatami\", \"deleted\", \"group\", \"system\", \"numcomp\", \"table\", \"wishsys\", \"pos1\", \"pos2\", \"pos3\", \"pos4\", \"pos5\", \"pos6\", \"pos7\", \"pos8\"";

int copy_table(sqlite3 *db, const char *tbl, const char *cols)
{
    char *zErrMsg = 0;
    char buf[256];
    int rc;

    if (!cols)
	snprintf(buf, sizeof(buf), "INSERT INTO other.%s SELECT * FROM main.%s",
		 tbl, tbl);
    else
	snprintf(buf, sizeof(buf), "INSERT INTO other.%s SELECT %s FROM main.%s",
		 tbl, cols, tbl);

    rc = sqlite3_exec(db, buf, NULL, NULL, &zErrMsg);
    if (rc) fprintf(stderr, "Cannot insert to %s: %s\n", tbl, sqlite3_errmsg(db));

    return rc;
}

#define BUFLEN 256

sqlite3 *open_out_db(const char *filename, int version, char *outname)
{
    sqlite3 *db;
    
    snprintf(outname, BUFLEN, "%s", filename);
    char *p = strrchr(outname, '.');
    if (p) snprintf(p, BUFLEN - ((int)(p - outname)), "-%d.shi", version);
    else {
	fprintf(stderr, "Invalid database name %s\n", filename);
	return NULL;
    }

    int rc = sqlite3_open(outname, &db);
    if (rc) {
	fprintf(stderr, "Can't open database %s: %s\n",
		outname, sqlite3_errmsg(db));
	sqlite3_close(db);
	return NULL;
    }

    return db;
}

int attach_db(sqlite3 *db, char *outname)
{
    char *zErrMsg = 0;
    int rc;
    char buf[BUFLEN];

    snprintf(buf, sizeof(buf), "ATTACH DATABASE '%s' AS other", outname);
    rc = sqlite3_exec(db, buf, NULL, NULL, &zErrMsg);
    if (rc) fprintf(stderr, "Cannot attach %s: %s\n", outname, sqlite3_errmsg(db));
    return rc;
}

int main(int argc, char *argv[])
{
    sqlite3 *db_in, *db_out;
    char *zErrMsg = 0;
    int rc, i, recover = 0;
    char buf[BUFLEN], outname[BUFLEN];
    char *inname = NULL;
    
    const char *table_competitors_30 = table_competitors_29;
    const char *table_matches_30 = table_matches_29;
    const char *table_info_30 = table_info_29;
    const char *table_catdef_30 = table_catdef_29;

#ifdef WIN32
    AllocConsole();
    freopen("CON", "w", stdout);
#endif
    
    if (argc < 2) {
	fprintf(stderr, "USAGE:\n");
	fprintf(stderr, "db-convert [-c] file.shi\n");
	fprintf(stderr, "  Creates a new database file that can be used with older JudoShiai releases.\n");
	fprintf(stderr, "  New file has a different name, for example file.shi -> file-29.shi.\n");
	fprintf(stderr, "  Option -c creates a copy without other conversions.\n");
	fprintf(stderr, "  It can be used as a rescue trial if the database seem to be corrupted.\n");
	return 1;
    }

    for (i = 1; i < argc; i++) {
	if (argv[i][0] == '-') {
	    if (argv[i][1] == 'c')
		recover = 1;
	} else
	    inname = argv[i];
    }

    printf("Opening %s\n", inname);
    rc = sqlite3_open(inname, &db_in);
    if (rc) {
	fprintf(stderr, "Can't open database: %s\n",
		sqlite3_errmsg(db_in));
	sqlite3_close(db_in);
	return -1;
    }

    rc = sqlite3_exec(db_in, "SELECT sql FROM sqlite_master", callback_tables, NULL, &zErrMsg);

    if (color_exists) {
	db_version = 30;
    }

    printf("DB version is %d.%d\n", db_version/10, db_version%10);
    if (!recover && db_version < 30) {
	printf("Cannot convert to an earlier version.\nWill make a backup.\n\n");
    }

    if (recover) {
	db_out = open_out_db(inname, 0, outname);
	printf("Making a copy %s\n", outname);

	if (db_version == 30) {
	    rc = sqlite3_exec(db_out, table_competitors_30, NULL, NULL, &zErrMsg);
	    rc = sqlite3_exec(db_out, table_categories_30, NULL, NULL, &zErrMsg);
	    rc = sqlite3_exec(db_out, table_matches_30, NULL, NULL, &zErrMsg);
	    rc = sqlite3_exec(db_out, table_info_30, NULL, NULL, &zErrMsg);
	    rc = sqlite3_exec(db_out, table_catdef_30, NULL, NULL, &zErrMsg);
	} else {
	    rc = sqlite3_exec(db_out, table_competitors_29, NULL, NULL, &zErrMsg);
	    rc = sqlite3_exec(db_out, table_categories_29, NULL, NULL, &zErrMsg);
	    rc = sqlite3_exec(db_out, table_matches_29, NULL, NULL, &zErrMsg);
	    rc = sqlite3_exec(db_out, table_info_29, NULL, NULL, &zErrMsg);
	    rc = sqlite3_exec(db_out, table_catdef_29, NULL, NULL, &zErrMsg);
	}
	sqlite3_close(db_out);

	attach_db(db_in, outname);

	copy_table(db_in, "competitors", NULL);
	copy_table(db_in, "categories", NULL);
	copy_table(db_in, "matches", NULL);
	copy_table(db_in, "info", NULL);
	copy_table(db_in, "catdef", NULL);

	sqlite3_exec(db_in, "DETACH other", NULL, NULL, &zErrMsg);
    } else if (db_version == 30) {
	db_out = open_out_db(inname, 29, outname);
	printf("Converting to version 2.9 (%s)\n", outname);

	rc = sqlite3_exec(db_out, table_competitors_29, NULL, NULL, &zErrMsg);
	rc = sqlite3_exec(db_out, table_categories_29, NULL, NULL, &zErrMsg);
	rc = sqlite3_exec(db_out, table_matches_29, NULL, NULL, &zErrMsg);
	rc = sqlite3_exec(db_out, table_info_29, NULL, NULL, &zErrMsg);
	rc = sqlite3_exec(db_out, table_catdef_29, NULL, NULL, &zErrMsg);
	sqlite3_close(db_out);

	attach_db(db_in, outname);

	copy_table(db_in, "competitors", NULL);
	copy_table(db_in, "categories", columns_categories_29);
	copy_table(db_in, "matches", NULL);
	copy_table(db_in, "info", NULL);
	copy_table(db_in, "catdef", NULL);

	sqlite3_exec(db_in, "DETACH other", NULL, NULL, &zErrMsg);
    }
    
 out:
    sqlite3_close(db_in);
    return 0;
}
