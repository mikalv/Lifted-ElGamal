#pragma once
#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <mie/elgamal.hpp>
#include <mie/ecparam.hpp>
#include <mie/fp.hpp>
#include <mie/ec.hpp>
#include <mie/gmp_util.hpp>
#include <cybozu/random_generator.hpp>

#if defined(_WIN64) || defined(__x86_64__)
	#define ELGAMAL_OPT_FP
#endif

namespace elgamal {

namespace local {
struct TagZn;
} // elgamal::local

typedef mie::FpT<mie::Gmp, local::TagZn> Zn;

} // elgamal

namespace elgamal_impl {

namespace local {

template<class T>
void saveOne(std::ostream& os, const std::string& name, const T& x)
{
	if (!(os << x)) throw cybozu::Exception("elgamal:saveOne") << name;
}

template<class T>
void loadOne(std::istream& is, const std::string& name, T& x)
{
	if (!(is >> x)) throw cybozu::Exception("elgamal:loadOne") << name;
}

struct TagFp;
typedef mie::FpT<mie::Gmp, TagFp> Fp;

} // local

template<class _Fp>
struct ElgamalT {
	typedef _Fp Fp;
	typedef mie::EcT<Fp> Ec;
	typedef elgamal::Zn Zn;
	typedef mie::ElgamalT<Ec, Zn> Elgamal;

	struct System {
		static const mie::EcParam *ecParam;
		static cybozu::RandomGenerator rg;
		/*
			init system
			@param param [in] string such as secp192k1
			@note NOT thread safe because setting global parameters of elliptic curve
		*/
		static inline void init(const std::string param)
		{
			ecParam = mie::getEcParam(param);
			Zn::setModulo(ecParam->n);
			Fp::setModulo(ecParam->p);
			Ec::setParam(ecParam->a, ecParam->b);
		}
		static inline void save(std::ostream& os)
		{
			os << ecParam->name;
		}
		static inline void load(std::istream& is)
		{
			std::string param;
			is >> param;
			init(param);
		}
	};

	struct CipherText : Elgamal::CipherText {
	};

	struct PublicKey : Elgamal::PublicKey {
		void save(const std::string& fileName) const
		{
			std::ofstream ofs(fileName.c_str(), std::ios::binary);
			System::save(ofs);
			local::saveOne(ofs, "publicKey", *this);
		}
		void load(const std::string& fileName)
		{
			std::ifstream ifs(fileName.c_str(), std::ios::binary);
			System::load(ifs);
			local::loadOne(ifs, "publicKey", *this);
		}
		void enc(CipherText& c, const Zn& m) const
		{
			Elgamal::PublicKey::enc(c, m, System::rg);
		}
		void enc(CipherText&c, int m) const
		{
			Elgamal::PublicKey::enc(c, Zn(m), System::rg);
		}
		void rerandomize(CipherText& c) const
		{
			Elgamal::PublicKey::rerandomize(c, System::rg);
		}
	};

	struct PrivateKey : Elgamal::PrivateKey {
		void init()
		{
			const mie::EcParam *ecParam = System::ecParam;
			const Fp x0(ecParam->gx);
			const Fp y0(ecParam->gy);
			Ec P(x0, y0);
			// Zn::getModBitLen() may be greater than ecParam->bitLen
			((typename Elgamal::PrivateKey*)this)->init(P, Zn::getModBitLen(), System::rg);
		}
		void save(const std::string& fileName) const
		{
			std::ofstream ofs(fileName.c_str(), std::ios::binary);
			System::save(ofs);
			local::saveOne(ofs, "privateKey", *this);
		}
		void load(const std::string& fileName)
		{
			std::ifstream ifs(fileName.c_str(), std::ios::binary);
			System::load(ifs);
			local::loadOne(ifs, "privateKey", *this);
		}
		const PublicKey& getPublicKey() const
		{
			const typename Elgamal::PublicKey& pub = Elgamal::PrivateKey::getPublicKey();
			return *reinterpret_cast<const PublicKey*>(&pub);
		}
	};
};

template<class Fp> const mie::EcParam* ElgamalT<Fp>::System::ecParam;
template<class Fp> cybozu::RandomGenerator ElgamalT<Fp>::System::rg;

} // elgamal_impl

#ifdef ELGAMAL_OPT_FP
#include <impl/elgamal_dispatch.hpp>
namespace elgamal {

typedef elgamal_disp::System System;
typedef elgamal_disp::CipherText CipherText;
typedef elgamal_disp::PublicKey PublicKey;
typedef elgamal_disp::PrivateKey PrivateKey;

} // elgamal
#else

namespace elgamal {

typedef elgamal_impl::ElgamalT<elgamal_impl::local::Fp> Elgamal;

typedef Elgamal::System System;
typedef Elgamal::CipherText CipherText;
typedef Elgamal::PublicKey PublicKey;
typedef Elgamal::PrivateKey PrivateKey;

} // elgamal
#endif