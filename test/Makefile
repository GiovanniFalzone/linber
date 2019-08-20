TARGET = linber_monitor
TARGET += test_service
TARGET += Calculator

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
