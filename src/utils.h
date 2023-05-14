#ifndef GBEMU_UTILS_H_
#define GBEMU_UTILS_H_

// xが区間[begin, end)に含まれるならtrue、そうでなければfalseを返す
template <class T1, class T2, class T3>
inline bool InRange(T1 x, T2 begin, T3 end) {
  return begin <= x && x < end;
}

#endif  // GBEMU_UTILS_H_
