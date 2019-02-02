#ifndef HASHMAP_H
#define HASHMAP_H

#include <set>
#include <map>
using namespace std;

unsigned int hashMapInsert (map<uint64_t, unsigned int> *hashMap, set<unsigned int> *hashValues, uint64_t orgKey, int isSet);
unsigned int hashMapFind (map<uint64_t, unsigned int> *hashMap, uint64_t orgKey);

#endif
