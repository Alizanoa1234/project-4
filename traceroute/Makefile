# שם הקובץ הסופי
TARGET = traceroute

# קובץ המקור
SRC = traceroute.c

# הקומפיילר והדגלים
CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c99 -pedantic

# כלל ברירת המחדל
all: $(TARGET)

# כלל לקימפול התוכנית
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# כלל לניקוי קבצים שנוצרו
clean:
	rm -f $(TARGET)

# כלל להרצה
run: $(TARGET)
	./$(TARGET)

# כלל PHONY כדי למנוע התנגשויות עם קבצים באותו שם
.PHONY: all clean run



#H+C:
# # שם הקובץ הסופי
# TARGET = traceroute

# # קבצי המקור והכותרת
# SRC = traceroute.c
# HDR = traceroute.h

# # הקומפיילר והדגלים
# CC = gcc
# CFLAGS = -Wall -Wextra -Werror -std=c99 -pedantic

# # כלל ברירת המחדל
# all: $(TARGET)

# # כלל לקימפול התוכנית
# $(TARGET): $(SRC) $(HDR)
# 	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# # כלל לניקוי קבצים שנוצרו
# clean:
# 	rm -f $(TARGET)

# # כלל להרצה
# run: $(TARGET)
# 	./$(TARGET)

# # כלל PHONY כדי למנוע התנגשויות עם קבצים באותו שם
# .PHONY: all clean run
