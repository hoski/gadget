#ifndef agebandmatrixptrvector_h
#define agebandmatrixptrvector_h

#include "agebandmatrix.h"

/**
 * \class AgeBandMatrixPtrVector
 * \brief This class implements a dynamic vector of AgeBandMatrix values
 */
class AgeBandMatrixPtrVector {
public:
  /**
   * \brief This is the default AgeBandMatrixPtrVector constructor
   */
  AgeBandMatrixPtrVector() { size = 0; v = 0; };
  /**
   * \brief This is the AgeBandMatrixPtrVector constructor for a specified size
   * \param sz is the size of the vector to be created
   * \note The elements of the vector will all be created, and set to zero
   */
  AgeBandMatrixPtrVector(int sz);
  /**
   * \brief This is the AgeBandMatrixPtrVector constructor for a specified size with an initial value
   * \param sz is the size of the vector to be created
   * \param initial is the initial value for all the entries of the vector
   */
  AgeBandMatrixPtrVector(int sz, AgeBandMatrix* initial);
  AgeBandMatrixPtrVector(int size1, int Minage, const IntVector& minl, const IntVector& size2);
  /**
   * \brief This is the AgeBandMatrixPtrVector constructor that create a copy of an existing AgeBandMatrixPtrVector
   * \param initial is the AgeBandMatrixPtrVector to copy
   */
  AgeBandMatrixPtrVector(const AgeBandMatrixPtrVector& initial);
  /**
   * \brief This is the AgeBandMatrixPtrVector destructor
   * \note This will free all the memory allocated to all the elements of the vector
   */
  ~AgeBandMatrixPtrVector();
  /**
   * \brief This will add new entries to the vector
   * \param add is the number of new entries to the vector
   * \param value is the value that will be entered for the new entries
   */
  void resize(int add, AgeBandMatrix* value);
  /**
   * \brief This will add new empty entries to the vector
   * \param add is the number of new entries to the vector
   * \note The new elements of the vector will be created, and set to zero
   */
  void resize(int add);
  void resize(int add, int minage, const IntVector& minl, const IntVector& size);
  void resize(int add, int minage, int minl, const PopInfoMatrix& matr);
  /**
   * \brief This will delete an entry from the vector
   * \param pos is the element of the vector to be deleted
   * \note This will free the memory allocated to the deleted element of the vector
   */
  void Delete(int pos);
  /**
   * \brief This will return the size of the vector
   * \return the size of the vector
   */
  int Size() const { return size; };
  /**
   * \brief This will return the value of an element of the vector
   * \param pos is the element of the vector to be returned
   * \return the value of the specified element
   */
  AgeBandMatrix& operator [] (int pos);
  /**
   * \brief This will return the value of an element of the vector
   * \param pos is the element of the vector to be returned
   * \return the value of the specified element
   */
  const AgeBandMatrix& operator [] (int pos) const;
  void Migrate(const DoubleMatrix& Migrationmatrix);
protected:
  /**
   * \brief This is the vector of AddressKeeper values
   */
  AgeBandMatrix** v;
  /**
   * \brief This is the size of the vector
   */
  int size;
};

#ifdef GADGET_INLINE
#include "agebandmatrixptrvector.icc"
#endif

#endif
