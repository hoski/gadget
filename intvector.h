#ifndef intvector_h
#define intvector_h

/**
 * \class IntVector
 * \brief This class implements a dynamic vector of int values
 */
class IntVector {
public:
  /**
   * \brief This is the default IntVector constructor
   */
  IntVector() { size = 0; v = 0; };
  /**
   * \brief This is the IntVector constructor for a specified size
   * \param sz is the size of the vector to be created
   * \note The elements of the vector will all be created, and set to zero
   */
  IntVector(int sz);
  /**
   * \brief This is the IntVector constructor for a specified size with an initial value
   * \param sz is the size of the vector to be created
   * \param initial is the initial value for all the entries of the vector
   */
  IntVector(int sz, int initial);
  /**
   * \brief This is the IntVector constructor that creates a copy of an existing IntVector
   * \param initial is the IntVector to copy
   */
  IntVector(const IntVector& initial);
  /**
   * \brief This is the IntVector destructor
   * \note This will free all the memory allocated to all the elements of the vector
   */
  ~IntVector();
  /**
   * \brief This will add new entries to the vector
   * \param add is the number of new entries to the vector
   * \param value is the value that will be entered for the new entries
   */
  void resize(int add, int value);
  /**
   * \brief This will add new empty entries to the vector
   * \param add is the number of new entries to the vector
   * \note The new elements of the vector will be created, and set to zero
   */
  void resize(int add);
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
  int& operator [] (int pos) { return v[pos]; };
  /**
   * \brief This will return the value of an element of the vector
   * \param pos is the element of the vector to be returned
   * \return the value of the specified element
   */
  const int& operator [] (int pos) const { return v[pos]; };
  IntVector& operator = (const IntVector& d);
protected:
  /**
   * \brief This is the vector of int values
   */
  int* v;
  /**
   * \brief This is the size of the vector
   */
  int size;
};

#endif
