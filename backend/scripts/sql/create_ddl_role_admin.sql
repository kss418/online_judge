SELECT format('CREATE ROLE %I WITH LOGIN PASSWORD %L', :'ddl_user', :'ddl_password')
WHERE NOT EXISTS(SELECT 1 FROM pg_roles WHERE rolname = :'ddl_user')
\gexec

SELECT format('ALTER ROLE %I WITH LOGIN PASSWORD %L', :'ddl_user', :'ddl_password')
\gexec

SELECT format('CREATE DATABASE %I', :'db_name')
WHERE NOT EXISTS(SELECT 1 FROM pg_database WHERE datname = :'db_name')
\gexec

GRANT CONNECT ON DATABASE :"db_name" TO :"ddl_user";
