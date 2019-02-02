#include <hashMap.h>
using namespace std;

unsigned int hashMapInsert (map<uint64_t, unsigned int> *hashMap, set<unsigned int> *hashValues, uint64_t orgKey, int isSet) {
	unsigned int hashValue;
	map<uint64_t, unsigned int>::iterator hashMapIter;
	set<unsigned int>::iterator hashValuesIter;
	static int setHit = 0;
	static int getHit = 0;
	if ((hashMapIter = hashMap->find(orgKey)) != hashMap->end()) {
		/*
		if (isSet) {
			printf("SET HIT: %d\n", ++setHit);
		} else {
			printf("GET HIT: %d\n", ++getHit);
		}
		*/
		return hashMapIter->second;
	}
	else {
		hashValue = std::hash<uint64_t>()(orgKey) & 0xffffffff;
		while ((hashValuesIter = hashValues->find(hashValue)) != hashValues->end()) {
			hashValue = std::hash<uint64_t>()(orgKey) & 0xffffffff;
		}
		hashValues->insert(hashValue);
		hashMap->insert(pair<uint64_t, unsigned int>(orgKey, hashValue));
		return hashValue;
	}
}

unsigned int hashMapFind (map<uint64_t, unsigned int> *hashMap, uint64_t orgKey) {
	unsigned int hashValue;
	map<uint64_t, unsigned int>::iterator hashMapIter;
	
	if ((hashMapIter = hashMap->find(orgKey)) != hashMap->end()) {
		return hashMapIter->second;
	}
	else {
		return -1;
	}

}
