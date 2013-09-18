/*
 * ci_string.h
 *
 *  Created by: Linus Mellberg
 *  12 sep 2013
 *
 *  Taken from http://stackoverflow.com/a/2886589/2752264
 *  Typedefs ci_string a string class that does case insensitive comparisons.
 */

#ifndef CI_STRING_H_
#define CI_STRING_H_

#include <string>
#include <type_traits>

struct ci_char_traits : public std::char_traits<char> {
	static bool eq(char c1, char c2) { return toupper(c1) == toupper(c2); }
	static bool ne(char c1, char c2) { return toupper(c1) != toupper(c2); }
	static bool lt(char c1, char c2) { return toupper(c1) <  toupper(c2); }
	static int compare(const char* s1, const char* s2, size_t n) {
		while( n-- != 0 ) {
			if( toupper(*s1) < toupper(*s2) ) return -1;
			if( toupper(*s1) > toupper(*s2) ) return 1;
			++s1; ++s2;
		}
		return 0;
	}
	static const char* find(const char* s, int n, char a) {
		while( n-- > 0 && toupper(*s) != toupper(a) ) {
			++s;
		}
		return s;
	}
};

typedef std::basic_string<char, ci_char_traits> ci_string;

#endif /* CI_STRING_H_ */
