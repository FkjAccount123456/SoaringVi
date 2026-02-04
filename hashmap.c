#include "hashmap.h"
#include <stdlib.h>
#include <string.h>

size_t get_str_hash(char *str, size_t len) {
  size_t hash = 0;
  for (size_t i = 0; i < len && str[i]; i++)
    hash = hash * 131 + str[i];
  return hash;
}

void hashmap_init(hashmap *obj, size_t reserved) {
  obj->len = 0;
  obj->max = reserved;
  obj->mod = reserved;
  obj->table = malloc(obj->mod * sizeof(size_t));
  obj->entrys = malloc(obj->max * sizeof(struct obj_entry));
  memset(obj->table, 0xff, obj->mod * sizeof(size_t));
}

void hashmap_free(hashmap *obj) {
  free(obj->table);
  free(obj->entrys);
}

void _hashmap_insert(hashmap *obj, char *key, void *val) {
  size_t hash = get_str_hash(key, -1);
  size_t pos = hash % obj->mod;
  if (obj->len >= obj->max) {
    obj->max <<= 1;
    obj->entrys = realloc(obj->entrys, obj->max * sizeof(struct obj_entry));
  }
  size_t i = pos;
  do {
    if (obj->table[i] == -1) {
      obj->entrys[obj->len].key = key;
      obj->entrys[obj->len].val = val;
      obj->entrys[obj->len].len = strlen(key);
      obj->entrys[obj->len].hash = hash;
      obj->table[i] = obj->len++;
      return;
    } else if (obj->entrys[obj->table[i]].hash == hash &&
               !strcmp(obj->entrys[obj->table[i]].key, key)) {
      obj->entrys[obj->table[i]].val = val;
      return;
    }
    i = (i + 1) % obj->mod;
  } while (i != pos);
  exit(1);
}

void _hashmap_insert_shortstr(hashmap *obj, size_t hash, void *val) {
  size_t pos = hash % obj->mod;
  if (obj->len >= obj->max) {
    obj->max <<= 1;
    obj->entrys = realloc(obj->entrys, obj->max * sizeof(struct obj_entry));
  }
  size_t i = pos;
  do {
    if (obj->table[i] == -1) {
      obj->entrys[obj->len].key = "";
      obj->entrys[obj->len].val = val;
      obj->entrys[obj->len].len = 0;
      obj->entrys[obj->len].hash = hash;
      obj->table[i] = obj->len++;
      return;
    } else if (obj->entrys[obj->table[i]].len == 0 &&
               obj->entrys[obj->table[i]].hash == hash) {
      obj->entrys[obj->table[i]].val = val;
      return;
    }
    i = (i + 1) % obj->mod;
  } while (i != pos);
  exit(1);
}

void _hashmap_expand(hashmap *obj) {
  obj->mod <<= 1;
  obj->table = realloc(obj->table, obj->mod * sizeof(struct obj_entry));
  memset(obj->table, 0xff, obj->mod * sizeof(size_t));
  size_t len = obj->len;
  obj->len = 0;
  for (size_t i = 0; i < len; i++)
    _hashmap_insert(obj, obj->entrys[i].key, obj->entrys[i].val);
}

void hashmap_insert(hashmap *obj, char *key, void *val) {
  if (obj->len * 2 > obj->mod)
    _hashmap_expand(obj);
  _hashmap_insert(obj, key, val);
}

void hashmap_insert_shortstr(hashmap *obj, size_t hash, void *val) {
  if (obj->len * 2 > obj->mod)
    _hashmap_expand(obj);
  _hashmap_insert_shortstr(obj, hash, val);
}

void **hashmap_get(hashmap *obj, char *key) {
  size_t hash = get_str_hash(key, -1);
  size_t pos = hash % obj->mod;
  size_t i = pos;
  do {
    if (obj->table[i] == -1)
      return NULL;
    else if (obj->entrys[obj->table[i]].hash == hash &&
             !strcmp(obj->entrys[obj->table[i]].key, key))
      return &obj->entrys[obj->table[i]].val;
    i = (i + 1) % obj->mod;
  } while (i != pos);
  return NULL;
}

void **hashmap_get_shortstr(hashmap *obj, size_t hash) {
  size_t pos = hash % obj->mod;
  size_t i = pos;
  do {
    if (obj->table[i] == -1)
      return NULL;
    else if (obj->entrys[obj->table[i]].len <= SHORTSTR_MAX &&
             obj->entrys[obj->table[i]].hash == hash)
      return &obj->entrys[obj->table[i]].val;
    i = (i + 1) % obj->mod;
  } while (i != pos);
  return NULL;
}
