OBJS=*.o
ODIR=obj_demo
$(ROOT_DIR)/$(BIN_DIR)/$(BIN):$(ODIR)/$(OBJS)
	$(CC) -o $(ROOT_DIR)/$(BIN_DIR)/test_processSend $(ODIR)/lib_infinite_buffer_test_processSend.o $(CFLAGS) $(LDFLAGS)
	$(CC) -o $(ROOT_DIR)/$(BIN_DIR)/test_processRecv $(ODIR)/lib_infinite_buffer_test_processRecv.o $(CFLAGS) $(LDFLAGS)