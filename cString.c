#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define DBGINFO


typedef struct cString {
    char *text;
    size_t len;
} cString;


size_t cString_len(cString *cStr) {
  return cStr->len - 1;
}


cString *cString_new() {
  cString *newStr = calloc(sizeof(cString), 1);
  if (!newStr) {
#ifdef DBGINFO
    printf("Failed to allocate cString\n");
#endif
    return NULL;
  }

  return newStr;
}


int cString_set(cString *string, char *text) {
  string->text = strdup(text);

  if (!string->text) {
#ifdef DBGINFO
    printf("Failed to strdup input: %s\n", text);
#endif
    return -1;
  }
  string->len = strlen(text) + 1;
  return string->len;
}

cString *cString_newStr(char *text) {
  cString *cStr = cString_new();
  if (!cStr) return NULL;

  cString_set(cStr, text);
  return cStr;
}


int cString_append(cString *string1, cString *string2) {
  size_t str1Len = cString_len(string1);
  size_t str2Len = cString_len(string2);
  size_t newLen = str1Len + str2Len + 1;

  char *resizedStr = realloc(string1->text, sizeof(char) * newLen);
  if (!resizedStr) {
#ifdef DBGINFO
    printf("Failed to realloc original string size of %zu to %zu\n", string1->len, string2->len);
#endif
    return -1;
  }

  string1->text = resizedStr;
  string1->len = newLen;
  strncpy((char*)&string1->text[str1Len], string2->text, str2Len);
  return string1->len;
}

void cString_free(cString **cStr) {
  if (!cStr || !*cStr) return;

  cString *cStrPtr = *cStr;
  free(cStrPtr->text);
  cStrPtr->text = NULL;
  cStrPtr->len = 0;

  free(cStrPtr);
  *cStr = NULL;
}



int main() {
  cString *firstString = cString_newStr("Hello, ");
  printf("First string: %s\n", firstString->text);

  cString *secondString = cString_newStr("World!");
  printf("Second string: %s\n", secondString->text);


  cString_append(firstString, secondString);
  printf("Appended: %s\n", firstString->text);

  cString_free(&firstString);
  cString_free(&secondString);
  return 0;
}