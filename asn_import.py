#!/usr/bin/env python



import os, sys
import psycopg2
import fileinput

# change these
dbname='...'
user='...'
password='...'
host='...'
tablename='pfx2asn'


try:
    connection = psycopg2.connect("host=%s dbname=%s user=%s password=%s" \
            % (host, dbname, user, password));
except:
    sys.exit('Unable to connect to the database')





# import the AS world
def import_raw():
    try:
        # earlier versions didn't have them separated
        import psycopg2.errorcodes
    except:
        pass

    cursor = connection.cursor()
    cursor.execute("begin")
    cursor.execute("delete from %s" % tablename)

    for line in fileinput.input():
        pfx, asnb, asn = line.split()
        try:
            cursor.execute("INSERT INTO %s VALUES ( %%s, %%s )" % tablename, [pfx, asn])
        except psycopg2.IntegrityError, e:
            print e
            if hasattr(psycopg2, 'errorcodes'):
                unique_violation = psycopg2.errorcodes.UNIQUE_VIOLATION
            else:
                unique_violation = '23505'
            if e.pgcode == unique_violation:
                print 'UNIQUE_VIOLATION for %r; rolling back' % pfx
                cursor.execute("rollback")
        except:
            raise
            sys.exit('insert failed for %s, %s' % (pfx, asn))

    cursor.execute("commit")
    cursor.execute("vacuum analyze %s" % tablename)

import_raw()

