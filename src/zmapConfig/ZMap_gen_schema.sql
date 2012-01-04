-- sample database for ZMap config
-- use 'sqlite <db-name> ".read thisfile;"' to create the schema
-- we use rowid extensively to access tables, no need to specify explicitly
-- ref to http://www.sqlite.org/autoinc.html and also http://www.sqlite.org/lang_createtable.html

-- experiment with totally generic database

-- generic table attribute
create table atom (name text);

-- generic value of attribute of table row
create table value (atom_id integer, table_id integer, value text);

-- generic table: line type and name to unique rowid
-- different types can have the same name
create table stuff (table_type integer, name text);


--create table column_order (column_id integer, position integer);

insert into atom values("width");
insert into atom values("bump-mode");

insert into atom values("style");

insert into stuff values(1,"basic");
insert into stuff values(1,"align");
insert into stuff values(2,"ensembl-variation");
insert into stuff values(2,"repeat");


insert into value values(1,1,20);
insert into value values(2,1,"overlap");
insert into value values(1,2,15);
insert into value values(2,2,"all");

insert into value values(3,3,1);	-- style for ensembl-variation is basic
insert into value values(3,4,2);	-- style for repeat is align
-- not easy to do this by lookup


select s.name,a.name,v.value from stuff as s,atom as a, value as v where v.table_id = s.rowid and a.rowid=v.atom_id;

-- prints out:
-- basic|width|20
-- basic|bump-mode|overlap
-- align|width|15
-- align|bump-mode|all

-- seems simple for reading out non-nested data
