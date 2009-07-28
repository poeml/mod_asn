-- better name for pfx: range?
-- easier to type, easier to read
--
-- Note: "as" is a reserved word in SQL

CREATE TABLE "pfx2asn" (
        "pfx" ip4r NOT NULL PRIMARY KEY,
        "asn" integer NOT NULL
);

CREATE INDEX "pfx2asn_pfx_key" ON pfx2asn USING gist (pfx);

