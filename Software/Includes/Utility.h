/** @file Utility.h
 * Gather various global utility functions.
 * @author Adrien RICCIARDI
 */
#ifndef H_UTILITY_H
#define H_UTILITY_H

//-------------------------------------------------------------------------------------------------
// Functions
//-------------------------------------------------------------------------------------------------
/** Convert an ASCIIZ string made of the [0-9A-Fa-f] character set to its binary representation.
 * @param Pointer_String The string to convert.
 * @param Pointer_Binary On output, contain the converted binary value.
 * @return 0 if the number was successfully converted,
 * @return 1 if the provided number is higher than the available output storage (4 bytes),
 * @return 2 if the provided string contains a non hexadecimal character.
 */
unsigned char UtilityConvertHexadecimalToBinary(char *Pointer_String, unsigned long *Pointer_Binary);

#endif
