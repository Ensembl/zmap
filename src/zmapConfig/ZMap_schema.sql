-- sample database for ZMap config
-- use 'sqlite <db-name> ".read thisfile;"' to create the schema
-- we use rowid extensively to access tables, no need to specify explicitly
-- ref to http://www.sqlite.org/autoinc.html and also http://www.sqlite.org/lang_createtable.html


-- generic table attribute
create table atom (name text);

-- generic value of attribute of table row
create table value (atom_id integer, table_type integer, table_id integer, value text);

-- feature styles, held via attributes
create table style (name text);

-- featuresets, with some mandatory columns
create table featureset
(
	set_type integer,	-- what kind of data eg align transcript held via atom table
	name text,			-- indexed? (we can have 1000's of these)
	description text,	-- words..
	server_id integer,	-- where to get it from
	column_id integer,	-- where to display it
	style_id			-- how to display it
);

create table zmap_column
(
	name text,
	description text,
	server_id integer,	-- (for ACEDB - requests are via column not featureset)
	style_id integer,	-- optional column wide options
	loadable integer,	-- appears in load columns dialog?
	group_id integer	-- ref to attribute, group columns together in load columns dialog
);

create table column_order (column_id integer, position integer);


create table server
(
	server_type integer,	-- eg ACEDB, Pipe, DAS...
	sub_type integer,		-- eg (for pipes) filter, bam_get, bam_coverage_get...
	name text,
	description text,
	url	text				-- base - no options included
);
