#ifndef _UTIL_H_
#define _UTIL_H_
#if defined(_MSC_VER)
#pragma once
#endif

/*! 
* \file util.h  
* \ingroup CIAM
* \brief A set of commonly used functions.
* 
* This is a set of functions which are frequently needed within the program.
* \author Josh Lurz
* \date $Date$
* \version $Revision$
*/

#include "util/base/include/definitions.h"
#include <limits>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <ctime>
#include <sstream>
#include <map>
#include <vector>
namespace util {

    /*! \brief Returns the value within this map associated with a given key. 
    * \details This function takes as its input a map and a key to search for. 
    * It will return the value associated with the key, or the default value for the class
    * of the object if the key is not found.
    * \note Use this function instead of recoding a map search, as this function should be more efficient and 
    * handle errors more appropriately. 
    * \todo Evaluate returning a const reference.
    * \param currMap The map within which to search for the value.
    * \param key The key to find the value with which it is associated.
    * \return The value in the currMap associated with the key, the default constructed object otherwise. 
    */
    template <class K, class V>
    const V searchForValue( const std::map<K,V>& currMap, const K& key ){
        V retValue;
        std::map<K,V>::const_iterator iter = currMap.find( key );
        if( iter != currMap.end() ){
            retValue = iter->second;
        } else {
            retValue = V(); //returns default constructor, 0 for doubles and ints
        }
        return retValue;
    }

    /*! \brief A function to determine the sign of a number.
    * \param number A templated parameter which must be comparable to 0.
    * \return Returns -1 if the number is less than 0, +1 otherwise.
    */
    template <class T>
    const int sign( const T number ) {
        return ( number < 0 )?(-1):(1);
    }

    /*! \brief A function to check for the validity of numbers. 
    *
    * Occasionally after calculations numbers are no longer in the range of
    * real numbers, however C++ will continue to perform calculations on them.
    * This function can then be used to check if numbers are not NaN or Infinity.
    *   
    * \warning Some compiler/platforms do not support these checks. In that case this function will always return true. 
    * \warning Do not try to perform this check without using this function. It will fail on some compilers.
    * \param number A templated parameter which must be comparable to 0.
    * \return Returns -1 if the number is less than 0, +1 otherwise.
    */
    template <class T>
    const bool isValidNumber( const T number ) {
        bool tempval = ( number == number );
        if ( std::numeric_limits<double>::infinity() != 0 ) {
            tempval = tempval && ( number != std::numeric_limits<T>::infinity() );
        }
        return tempval;
    }

    /*! \brief A function to determine if two doubles are equal.
    *
    * Due to inaccuracies in machine arithmatic, it is virtually impossible for two doubles with decimal values
    * that are calculated with different methods to be exactly equal. This function checks if the two values
    * are within a very small threshhold. 
    *
    * \warning Do not compare two doubles using the == operator. Use this function instead. 
    * \param firstNumber The first double to compare.
    * \param secondNumber The second double to compare.
    * \return Whether the two doubles are within SMALL_NUM of equivalence. 
    */
    inline const bool isEqual( const double firstNumber, const double secondNumber ) {
        const double SMALL_NUM = 1E-10;
        return ( fabs( firstNumber - secondNumber ) < SMALL_NUM );
    }

    /*! \brief A function to check if a file was opened successfully.
    *
    * When C++ opens a file, for input or output, it does not check whether it was opened successfully.
    * If the file has not been opened correctly, the program will often not fail as FORTRAN programs do,
    * but instead it will have unexpected behavior. This function will check if the file is open, and 
    * if it is not it will print an error message before calling abort. 
    * 
    * \todo This function should be more flexible, for use in cases where the missing file is a non-fatal problem.
    * \param streamIn A templated parameter that must have the function is_open. This was done so that one function could 
    * check both input files and output files.
    * \param fName The name of the file streamIn references, so that the error message can be more informative.
    */
    template <class T>
    inline void checkIsOpen( T& streamIn, const std::string& fName ) {
        if( !streamIn.is_open() ) {
            std::cerr << "Severe Error: File " << fName << " could not be opened." << std::endl;
            abort();
        }
    }

    /*! \brief A function to replace spaces with underscores.
    *
    * This function will replace all spaces in a string with underscores. Each space will be replaced by 
    * exactly one underscore, multiple spaces are not concatonated. Other whitespace characters are not
    * replaced. 
    * 
    * \todo This function currently generates a compiler warning, which should be fixed. 
    * \param stringIn The string in which spaces should be replaced by underscores.
    */
    inline void replaceSpaces( std::string& stringIn ) {
        static const std::basic_string<char>::size_type npos = -1;
        std::basic_string <char>::size_type index;

        while( stringIn.find_first_of( " " ) != npos ) {
            index = stringIn.find_first_of( " " );
            stringIn.replace( index, 1, "_" );
        }
    }

    /*! \brief Static function which returns SMALL_NUM. 
    * \details This is a static function which is used to find the value of the constant SMALL_NUM.
    * This avoids the initialization problems of static variables. This function should be used instead
    * of defining this constant in multiple locations in the code.
    * \return The constant SMALL_NUM.
    */
   static inline double getSmallNumber() {
      const double SMALL_NUM = 1e-6;
      return SMALL_NUM;
   }

    /*! \brief Static function which returns VERY_SMALL_NUM. 
    * \details This is a static function which is used to find the value of the constant VERY_SMALL_NUM.
    * This avoids the initialization problems of static variables. This function should be used instead
    * of defining this constant in multiple locations in the code.
    * \return The constant VERY_SMALL_NUM.
    */
   static inline double getVerySmallNumber() {
      const double VERY_SMALL_NUM = 1e-8;
      return VERY_SMALL_NUM;
   }

    /*! \brief Static function which returns EXTREMELY_SMALL_NUM. 
    * \details This is a static function which is used to find the value of the constant EXTREMELY_SMALL_NUM.
    * This avoids the initialization problems of static variables. This function should be used instead
    * of defining this constant in multiple locations in the code.
    * \return The constant EXTREMELY_SMALL_NUM.
    */
   static inline double getTinyNumber() {
      const double EXTREMELY_SMALL_NUM = 1e-16;
      return EXTREMELY_SMALL_NUM;
   }

    /*! \brief Static function which returns LARGE_NUM. 
    * \details This is a static function which is used to find the value of the constant LARGE_NUM.
    * This avoids the initialization problems of static variables. This function should be used instead
    * of defining this constant in multiple locations in the code.
    * \return The constant LARGE_NUM.
    */
   static inline double getLargeNumber() {
      const double LARGE_NUM = 1e+6;
      return LARGE_NUM;
   }

    /*! \brief Function which creates an XML compliant date time string.
    *
    * This function takes as an argument a time_t object and returns a string containing the date and time in the following format:
    * yyyy-mm-dd-Thh:mm-GMTOFFSET
    * ie: 2003-01-11T09:30:47-05:00
    * \param time time_t to convert to XML string form.
    * \return string The time converted to XML date string format.
    * \bug GMT offset does not work properly.
    */
   static std::string XMLCreateDate( const time_t& time ) {
       std::stringstream buffer;
       std::string retString;
       struct tm* timeInfo;
       struct tm* umtTimeInfo;
	
	    timeInfo = localtime( &time );
	    umtTimeInfo = gmtime( &time );
	
	    // Create the string
	    buffer << ( timeInfo->tm_year + 1900 ); // Set the year
	    buffer << "-";
	    buffer << timeInfo->tm_mday; // Set the day
	    buffer << "-";
	    buffer << ( timeInfo->tm_mon + 1 ); // Month's in ctime range from 0-11
	    buffer << "T";
	    buffer << timeInfo->tm_hour;
	    buffer << ":";
	    buffer << timeInfo->tm_min;
	    buffer << ":";
	    buffer << timeInfo->tm_sec;
	    buffer << "-";
	
	    int umtDiff = timeInfo->tm_hour - umtTimeInfo->tm_hour;
	    if( umtDiff < 10 ) {
		    buffer << "0";
	    }
	    buffer << umtDiff;
	    buffer << ":00";
	    // Completed creating the string;
	    buffer >> retString;
	
	    return retString;
    }
} // End util namespace.
#endif // _UTIL_H_
