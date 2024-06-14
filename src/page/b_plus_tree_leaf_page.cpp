#include "page/b_plus_tree_leaf_page.h"

#include <algorithm>

#include "index/generic_key.h"

#define pairs_off (data_)
#define pair_size (GetKeySize() + sizeof(RowId))
#define key_off 0
#define val_off GetKeySize()
/*****************************************************************************
 * HELPER METHODS AND UTILITIES
 *****************************************************************************/

/**
 * TODO: Student Implement
 */
/**
 * Init method after creating a new leaf page
 * Including set page type, set current size to zero, set page id/parent id, set
 * next page id and set max size
 * 未初始化next_page_id
 */
void LeafPage::Init(page_id_t page_id, page_id_t parent_id, int key_size, int max_size) {
  SetPageType(IndexPageType::LEAF_PAGE);
  SetSize(0);
  SetPageId(page_id);
  SetParentPageId(parent_id);
  SetMaxSize(max_size);
  SetKeySize(key_size);
  next_page_id_ = INVALID_PAGE_ID;
}

/**
 * Helper methods to set/get next page id
 */
page_id_t LeafPage::GetNextPageId() const {
  return next_page_id_;
}

void LeafPage::SetNextPageId(page_id_t next_page_id) {
  next_page_id_ = next_page_id;
  if (next_page_id == 0) {
    LOG(INFO) << "Fatal error";
  }
}

/**
 * TODO: Student Implement
 */
/**
 * Helper method to find the first index i so that pairs_[i].first >= key
 * NOTE: This method is only used when generating index iterator
 * 二分查找
 */
int LeafPage::KeyIndex(const GenericKey *key, const KeyManager &KM) {
  int left = 1;  // Start from the second key
  int right = GetSize() - 1;  // GetSize() returns the number of keys
  int res = INVALID_PAGE_ID;

  while (left <= right) {
    int mid = (left + right) / 2;
    GenericKey *mid_key = KeyAt(mid);

    int cmp = KM.CompareKeys(mid_key, key);
    if (cmp == 0) {
      // Key is found, return the corresponding page_id
      return mid;
    } else if (cmp < 0) {
      left = mid + 1;
    } else {
      res = mid;
      right = mid - 1;
    }
  }
  // Key is not found, return the page_id of the child that could contain the key
  return res;
}

/*
 * Helper method to find and return the key associated with input "index"(a.k.a
 * array offset)
 */
GenericKey *LeafPage::KeyAt(int index) {
  return reinterpret_cast<GenericKey *>(pairs_off + index * pair_size + key_off);
}

void LeafPage::SetKeyAt(int index, GenericKey *key) {
  memcpy(pairs_off + index * pair_size + key_off, key, GetKeySize());
}

RowId LeafPage::ValueAt(int index) const {
  return *reinterpret_cast<const RowId *>(pairs_off + index * pair_size + val_off);
}

void LeafPage::SetValueAt(int index, RowId value) {
  *reinterpret_cast<RowId *>(pairs_off + index * pair_size + val_off) = value;
}

void *LeafPage::PairPtrAt(int index) {
  return KeyAt(index);
}

void LeafPage::PairCopy(void *dest, void *src, int pair_num) {
  memcpy(dest, src, pair_num * (GetKeySize() + sizeof(RowId)));
}
/*
 * Helper method to find and return the key & value pair associated with input
 * "index"(a.k.a. array offset)
 */
std::pair<GenericKey *, RowId> LeafPage::GetItem(int index) { return {KeyAt(index), ValueAt(index)}; }

/*****************************************************************************
 * INSERTION
 *****************************************************************************/
/*
 * Insert key & value pair into leaf page ordered by key
 * @return page size after insertion
 */
int LeafPage::Insert(GenericKey *key, const RowId &value, const KeyManager &KM) {
  // Find the position to insert the new key-value pair
  if (GetSize() >= GetMaxSize()) return GetSize() + 1;
  int index = KeyIndex(key, KM);
  if (index == INVALID_PAGE_ID) {
    index = GetSize();
  } else {
    // Move all pairs in the current page one position to the right from the index
    PairCopy(PairPtrAt(index + 1), PairPtrAt(index), GetSize() - index);
  }
  // Insert the new key-value pair at the found position
  SetKeyAt(index, key);
  SetValueAt(index, value);
  // Update the size of the current page
  IncreaseSize(1);
  // Return the new size of the page
  return GetSize();
}

/*****************************************************************************
 * SPLIT
 *****************************************************************************/
/*
 * Remove half of key & value pairs from this page to "recipient" page
 */
void LeafPage::MoveHalfTo(LeafPage *recipient) {
  int half = GetMinSize();
  SetSize(half);
  recipient->CopyNFrom(PairPtrAt(half), GetMaxSize() - half);
}

/*
 * Copy starting from items, and copy {size} number of elements into me.
 */
void LeafPage::CopyNFrom(void *src, int size) {
  // Copy the data from the source address to the target address in the current page
  PairCopy(PairPtrAt(GetSize()),src,size);
  // Update the size of the current page
  IncreaseSize(size);
}

/*****************************************************************************
 * LOOKUP
 *****************************************************************************/
/*
 * For the given key, check to see whether it exists in the leaf page. If it
 * does, then store its corresponding value in input "value" and return true.
 * If the key does not exist, then return false
 */
bool LeafPage::Lookup(const GenericKey *key, RowId &value, const KeyManager &KM) {
  // Find the index of the key in the leaf page
  int index = KeyIndex(key, KM);
  // If the index is valid and the key at the index is equal to the given key
  if (index < GetSize() && index >= 0 && KM.CompareKeys(KeyAt(index), key) == 0) {
    // Store the value at the index in "value" and return true
    value = ValueAt(index);
    return true;
  }
  // If the key does not exist, return false
  return false;
}

/*****************************************************************************
 * REMOVE
 *****************************************************************************/
/*
 * First look through leaf page to see whether delete key exist or not. If
 * existed, perform deletion, otherwise return immediately.
 * NOTE: store key&value pair continuously after deletion
 * @return  page size after deletion
 */
int LeafPage::RemoveAndDeleteRecord(const GenericKey *key, const KeyManager &KM) {
  // Find the index of the key in the leaf page
  int index = KeyIndex(key, KM);
  // If the index is valid and the key at the index is equal to the given key
  if (index < GetSize() && index >= 0 && KM.CompareKeys(KeyAt(index), key) == 0) {
    // Move all pairs in the current page one position to the left from the index
    PairCopy(PairPtrAt(index), PairPtrAt(index + 1), GetSize() - index - 1);
    // Decrease the size of the current page
    IncreaseSize(-1);
  }
  // Return the size of the page
  return GetSize();
}

/*****************************************************************************
 * MERGE
 *****************************************************************************/
/*
 * Remove all key & value pairs from this page to "recipient" page. Don't forget
 * to update the next_page id in the sibling page
 */
void LeafPage::MoveAllTo(LeafPage *recipient) {
  // Copy all pairs from the current page to the end of the recipient page
  recipient->CopyNFrom(PairPtrAt(0), GetSize());
  // Set the next_page_id of the recipient page to the next_page_id of the current page
  recipient->SetNextPageId(GetNextPageId());
  // Set the size of the current page to 0
  SetSize(0);
}

/*****************************************************************************
 * REDISTRIBUTE
 *****************************************************************************/
/*
 * Remove the first key & value pair from this page to "recipient" page.
 *
 */
void LeafPage::MoveFirstToEndOf(LeafPage *recipient) {
  // Copy the first key-value pair from the current page to the end of the recipient page
  recipient->CopyLastFrom(KeyAt(0), ValueAt(0));
  // Move all pairs in the current page one position to the left
  PairCopy(PairPtrAt(0), PairPtrAt(1), GetSize() - 1);
  // Decrease the size of the current page
  IncreaseSize(-1);
}

/*
 * Copy the item into the end of my item list. (Append item to my array)
 */
void LeafPage::CopyLastFrom(GenericKey *key, const RowId value) {
  SetKeyAt(GetSize(), key);
  SetValueAt(GetSize(), value);
  IncreaseSize(1);
}

/*
 * Remove the last key & value pair from this page to "recipient" page.
 */
void LeafPage::MoveLastToFrontOf(LeafPage *recipient) {
  // Copy the last key-value pair from the current page to the beginning of the recipient page
  recipient->CopyFirstFrom(KeyAt(GetSize() - 1), ValueAt(GetSize() - 1));
  // Decrease the size of the current page
  IncreaseSize(-1);
}

/*
 * Insert item at the front of my items. Move items accordingly.
 *
 */
void LeafPage::CopyFirstFrom(GenericKey *key, const RowId value) {
  PairCopy(PairPtrAt(1),PairPtrAt(0),GetSize());
  SetKeyAt(0, key);
  SetValueAt(0, value);
  IncreaseSize(1);
}