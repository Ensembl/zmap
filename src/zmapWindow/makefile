#
# makes the application part of the zmap.
#

CC = cc
CFLAGS = -g -pthread -std -msg_enable level3


# ZMAP code

APP_NAME = zmap

APP_LIB = libzmap.a

APP_HDRS = seqregion.h stringbucket.h zmapcontrol.h zmap.h

APP_OBJS = seqregion.o seqregionconvert.o stringbucket.o  \
	zmapbccol.o zmapcols.o zmapcontrol.o zmapsequence.o

# Need gtk and our own include files so specify a rule for  .c -> .o
.c.o :
	$(CC) -g `gtk-config --cflags` -I. -c $<  

all: $(APP_NAME)

clean:
	rm -f $(APP_NAME) $(APP_OBJ)

$(APP_OBJS): $(APP_HDRS)

$(APP_LIB): $(APP_OBJS)
	ar ru $@ $?

$(APP_NAME): $(APP_OBJS)
	$(CC) $(CFLAGS) -o $@ `gtk-config --libs` $(APP_OBJS) 

########################## end of makefile ##################################
