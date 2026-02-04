/*
算了，懒得重新实现一个了，直接复制TinyScript的吧
*/
#ifndef HASHMAP_H
#define HASHMAP_H

#include "debug.h"
#include <stddef.h>

struct obj_entry {
  size_t len;
  size_t hash;
  char *key;
  void *val;
};

typedef struct hashmap {
  size_t *table;
  struct obj_entry *entrys;
  size_t len, max, mod;
} hashmap;

#define SHORTSTR_MAX 9

size_t get_str_hash(char *str, size_t len);

void hashmap_init(hashmap *obj, size_t reserved);
void hashmap_free(hashmap *obj);
void hashmap_insert(hashmap *obj, char *key, void *val);
void hashmap_insert_shortstr(hashmap *obj, size_t hash, void *val);
void **hashmap_get(hashmap *obj, char *key);
void **hashmap_get_shortstr(hashmap *obj, size_t hash);

#endif // HASHMAP_H
