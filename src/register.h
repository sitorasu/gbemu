#ifndef GBEMU_REGISTER_H_
#define GBEMU_REGISTER_H_

#include <string>

namespace gbemu {

// レジスタに対する操作を規定するインターフェース。
// UIntTypeはレジスタのビット幅を表す符号無し整数型。
template <class UIntType>
class Register {
 public:
  Register(std::string name) : name_(name) {}
  virtual UIntType get() = 0;
  virtual void set(UIntType data) = 0;
  std::string name() { return name_; }

 private:
  std::string name_;
};

// 単一のレジスタの実装。
// UIntTypeはレジスタのビット幅を表す符号無し整数型。
template <class UIntType>
class SingleRegister : public Register<UIntType> {
 public:
  SingleRegister(std::string name) : Register<UIntType>(name) {}
  UIntType get() override { return data_; }
  void set(UIntType data) override { data_ = data; }

 protected:
  UIntType data_{};
};

// レジスタのペアの実装
// SubUIntはペアを構成するサブレジスタのビット幅を表す符号無し整数型。
// SuperUIntは全体のビット幅を表す符号無し整数型。
template <class SubUInt, class SuperUInt>
class RegisterPair : public Register<SuperUInt> {
  static_assert(sizeof(SuperUInt) == sizeof(SubUInt) * 2,
                "The size of the pair register must be twice the size of the "
                "sub register.");

 public:
  // 名前を指定しなければ上位と下位のサブレジスタの名前を結合した名前とする。
  RegisterPair(Register<SubUInt>& upper, Register<SubUInt>& lower)
      : Register<SuperUInt>(upper.name() + lower.name()),
        upper_(upper),
        lower_(lower) {}
  // 名前を明示的に指定することもできる。
  RegisterPair(Register<SubUInt>& upper, Register<SubUInt>& lower,
               std::string name)
      : Register<SuperUInt>(name), upper_(upper), lower_(lower) {}
  SuperUInt get() override {
    return (static_cast<SuperUInt>(upper_.get()) << (sizeof(SubUInt) * 8)) |
           lower_.get();
  }
  void set(SuperUInt data) override {
    lower_.set(static_cast<SubUInt>(data));
    upper_.set(data >> (sizeof(SubUInt) * 8));
  }

 private:
  Register<SubUInt>& upper_{};
  Register<SubUInt>& lower_{};
};

}  // namespace gbemu

#endif  // GBEMU_REGISTER_H_
