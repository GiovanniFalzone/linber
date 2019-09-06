TARGET = linber_monitor
TARGET += test_service
TARGET += test_service_shm
TARGET += Calculator
TARGET += test_efficiency

.PHONY:
all: $(TARGET)
	for d in $(TARGET); \
	do                               \
		$(MAKE) --directory=$$d all; \
	done


.PHONY:
clean:
	for d in $(TARGET); \
	do                               \
		$(MAKE) --directory=$$d clean; \
	done
