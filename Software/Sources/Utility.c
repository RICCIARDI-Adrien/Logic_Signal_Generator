/** @file Utility.c
 * See Utility.h for description.
 * @author Adrien RICCIARDI
 */
#include <Utility.h>

//-------------------------------------------------------------------------------------------------
// Public functions
//-------------------------------------------------------------------------------------------------
unsigned char UtilityConvertHexadecimalToBinary(char *Pointer_String, unsigned long *Pointer_Binary)
{
	unsigned long Result = 0;
	unsigned char Nibble, Maximum_Nibbles_Count = 0;
	char Character;

	// Do not exceed the result storage size if the provided string is too long
	while (Maximum_Nibbles_Count <= (sizeof(Result) * 2)) // There are two 4-bit digits in each byte
	{
		// Cache the next character access
		Character = *Pointer_String;

		// Is the input string terminated ?
		if (Character == 0)
		{
			*Pointer_Binary = Result;
			return 0;
		}

		// Make sure that the character is valid
		if ((Character >= '0') && (Character <= '9')) Nibble = Character - '0';
		else if ((Character >= 'A') && (Character <= 'F')) Nibble = Character - 'A' + 10; // Add ten because such letter represent the decimal values 10 to 15
		else if ((Character >= 'a') && (Character <= 'f')) Nibble = Character - 'a' + 10; // Add ten because such letter represent the decimal values 10 to 15
		else return 2; // A non-allowed character has been found

		// Append the nibble at the end of the number
		Result <<= 4;
		Result |= Nibble;

		// Check the next character
		Pointer_String++;
		Maximum_Nibbles_Count++;
	}

	// The provided string exceeds the storage size
	return 1;
}
