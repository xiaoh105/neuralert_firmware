/****************************************************************************
 * The confidential and proprietary information contained in this file may    *
 * only be used by a person authorised under and to the extent permitted      *
 * by a subsisting licensing agreement from ARM Limited or its affiliates.    *
 *  (C) COPYRIGHT [2001-2017] ARM Limited or its affiliates.         *
 *      ALL RIGHTS RESERVED                          *
 * This entire notice must be reproduced on all copies of this file           *
 * and copies of this file may only be made by a person if such person is     *
 * permitted to do so under the terms of a subsisting license agreement       *
 * from ARM Limited or its affiliates.                       *
 *****************************************************************************/



/************* Include Files ****************/
#include "cc_pal_types.h"
#include "cc_pal_error.h"
#include "cc_pal_mem.h"
#include "cal.h"

/************************ Defines ******************************/

/************************ Enums ******************************/


/************************ Typedefs ******************************/


/************************ Global Data ******************************/

/************************ Private Functions ******************************/


/************************ Public Functions ******************************/

/**
 * @brief This function purpose is to perform secured memory comparison between two given
 *        buffers according to given size. The function will compare each byte till aSize
 *        number of bytes was compared even if the bytes are different.
 *        The function should be used to avoid security timing attacks.
 *
 *
 * @param[in] aTarget - The target buffer to compare
 * @param[in] aSource - The Source buffer to compare to
 * @param[in] aSize - Number of bytes to compare
 *
 * @return The function will return CC_SUCCESS in case of success, else errors from
 *         cc_pal_error.h will be returned.
 */
CCError_t CC_PalSecMemCmp(  const uint8_t* aTarget,
                        const uint8_t* aSource,
                        size_t  aSize       )
{
  /* internal index */
  uint32_t i = 0;

  /* error return */
  uint32_t error = CC_SUCCESS;

  /*------------------
      CODE
  -------------------*/

  /* Go over aTarget till aSize is reached (even if its not equal) */
  for (i = 0; i < aSize; i++)
  {
    if (aTarget[i] != aSource[i])
    {
      if (error != CC_SUCCESS)
        continue;
      else
      {
        if (aTarget[i] < aSource[i])
          error = CC_PAL_MEM_BUF2_GREATER;
        else
          error = CC_PAL_MEM_BUF1_GREATER;
      }
    }
  }

  return error;
}/* End of CC_PalSecMemCmp */


int32_t CC_PalMemCmpPlat(  const void* aTarget, /*!< [in] The target buffer to compare. */
                                  const void* aSource, /*!< [in] The Source buffer to compare to. */
                                  size_t      aSize    /*!< [in] Number of bytes to compare. */)
{
    return CRYPTO_MEMCMP(aTarget, aSource, aSize);

}/* End of CC_PalMemCmpPlat */

void* CC_PalMemCopyPlat(     void* aDestination, /*!< [out] The destination buffer to copy bytes to. */
                                      const void* aSource,      /*!< [in] The Source buffer to copy from. */
                                      size_t      aSize     /*!< [in] Number of bytes to copy. */ ){
    return memmove( aDestination,  aSource, aSize);
}/* End of CC_PalMemCopyPlat */


/*!
 * @brief This function purpose is to copy aSize bytes from source buffer to destination buffer.
 * This function Supports overlapped buffers.
 *
 * @return void.
 */
void CC_PalMemMovePlat(   void* aDestination, /*!< [out] The destination buffer to copy bytes to. */
                                  const void* aSource,      /*!< [in] The Source buffer to copy from. */
                                  size_t      aSize     /*!< [in] Number of bytes to copy. */)
{
    memmove(aDestination, aSource, aSize);
}/* End of CC_PalMemMovePlat */


/*!
 * @brief This function purpose is to set aSize bytes in the given buffer with aChar.
 *
 * @return void.
 */
void CC_PalMemSetPlat(   void* aTarget, /*!< [out]  The target buffer to set. */
                                 uint8_t aChar, /*!< [in] The char to set into aTarget. */
                                 size_t        aSize  /*!< [in] Number of bytes to set. */)
{
    CRYPTO_MEMSET(aTarget, aChar, aSize);
}/* End of CC_PalMemSetPlat */

/*!
 * @brief This function purpose is to set aSize bytes in the given buffer with zeroes.
 *
 * @return void.
 */
void CC_PalMemSetZeroPlat(    void* aTarget, /*!< [out]  The target buffer to set. */
                                     size_t      aSize    /*!< [in] Number of bytes to set. */)
{
    CRYPTO_MEMSET(aTarget, 0x00, aSize);
}/* End of CC_PalMemSetZeroPlat */

/*!
 * @brief This function purpose is to allocate a memory buffer according to aSize.
 *
 *
 * @return The function returns a pointer to allocated buffer or NULL if allocation failed.
 */
void* CC_PalMemMallocPlat(size_t  aSize /*!< [in] Number of bytes to allocate. */)
{
    return CRYPTO_MALLOC(aSize);
}/* End of CC_PalMemMallocPlat */

/*!
 * @brief This function purpose is to reallocate a memory buffer according to aNewSize.
 *        The content of the old buffer is moved to the new location.
 *
 * @return The function returns a pointer to the newly allocated buffer or NULL if allocation failed.
 */
void* CC_PalMemReallocPlat(  void* aBuffer,     /*!< [in] Pointer to allocated buffer. */
                                     size_t  aNewSize   /*!< [in] Number of bytes to reallocate. */)
{
	void *newalloc;
	newalloc = CRYPTO_MALLOC(aNewSize);
	if( newalloc != NULL ){
		memcpy( newalloc, aBuffer, (aNewSize));
		CRYPTO_FREE(aBuffer);
		return newalloc;
	}
	return NULL;
}/* End of CC_PalMemReallocPlat */

/*!
 * @brief This function purpose is to free allocated buffer.
 *
 *
 * @return void.
 */
void CC_PalMemFreePlat(void* aBuffer /*!< [in] Pointer to allocated buffer.*/)
{
    CRYPTO_FREE(aBuffer);
}/* End of CC_PalMemFreePlat */
