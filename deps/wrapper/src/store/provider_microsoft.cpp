#include "../stdafx.h"

#include "wrapper/store/provider_microsoft.h"

ProviderMicrosoft::ProviderMicrosoft(){
	LOGGER_FN();

	try{
		type = new std::string("MICROSOFT");
		providerItemCollection = new PkiItemCollection();

		init();
	}
	catch (Handle<Exception> &e){
		THROW_EXCEPTION(0, ProviderMicrosoft, e, "Cannot be constructed ProviderMicrosoft");
	}
}
void ProviderMicrosoft::init(){
	LOGGER_FN();

	try{
		std::string listStore[] = {
			"MY",
			"AddressBook",
			"ROOT",
			"TRUST",
			"CA",
			"Request",
		};

		HCERTSTORE hCertStore;

		for (int i = 0, c = sizeof(listStore) / sizeof(*listStore); i < c; i++){
			std::wstring widestr = std::wstring(listStore[i].begin(), listStore[i].end());
			hCertStore = CertOpenStore(
				CERT_STORE_PROV_SYSTEM,
				PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
				NULL,
				CERT_SYSTEM_STORE_CURRENT_USER,
				widestr.c_str()
				);

			if (!hCertStore) {
				LOGGER_ERROR("error open store");
				continue;
			}

			enumCertificates(hCertStore, &listStore[i]);
			enumCrls(hCertStore, &listStore[i]);

			CertCloseStore(hCertStore, 0);
		}
	}
	catch (Handle<Exception> &e){
		THROW_EXCEPTION(0, ProviderMicrosoft, e, "Error init microsoft provider");
	}
}

void ProviderMicrosoft::enumCertificates(HCERTSTORE hCertStore, std::string *category){
	LOGGER_FN();

	try{
		X509 *cert = NULL;
		const unsigned char *p;

		if (!hCertStore){
			THROW_EXCEPTION(0, ProviderMicrosoft, NULL, "Certificate store not opened");
		}

		PCCERT_CONTEXT pCertContext = NULL;
		do
		{
			pCertContext = CertEnumCertificatesInStore(hCertStore, pCertContext);
			if (pCertContext){
				p = pCertContext->pbCertEncoded;
				LOGGER_OPENSSL(d2i_X509);
				if (!(cert = d2i_X509(NULL, &p, pCertContext->cbCertEncoded))) {
					THROW_OPENSSL_EXCEPTION(0, ProviderMicrosoft, NULL, "'d2i_X509' Error decode len bytes");
				}

				Handle<Certificate> hcert = new Certificate(cert);
				Handle<PkiItem> item = objectToPKIItem(hcert);
				item->category = new std::string(*category);
				item->certificate = hcert;
				item->certKey = hasPrivateKey(pCertContext) ? new std::string("1") : new std::string("");

				providerItemCollection->push(item);
			}
		} while (pCertContext != NULL);
	}
	catch (Handle<Exception> &e){
		THROW_EXCEPTION(0, ProviderMicrosoft, e, "Error enum certificates in store");
	}
}

void ProviderMicrosoft::enumCrls(HCERTSTORE hCertStore, std::string *category){
	LOGGER_FN();

	try{
		X509_CRL *crl = NULL;
		const unsigned char *p;

		if (!hCertStore){
			THROW_EXCEPTION(0, ProviderMicrosoft, NULL, "Certificate store not opened");
		}

		PCCRL_CONTEXT pCrlContext = NULL;
		do
		{
			pCrlContext = CertEnumCRLsInStore(hCertStore, pCrlContext);
			if (pCrlContext){
				p = pCrlContext->pbCrlEncoded;
				LOGGER_OPENSSL(d2i_X509_CRL);
				if (!(crl = d2i_X509_CRL(NULL, &p, pCrlContext->cbCrlEncoded))) {
					THROW_OPENSSL_EXCEPTION(0, ProviderMicrosoft, NULL, "'d2i_X509_CRL' Error decode len bytes");
				}

				Handle<CRL> hcrl = new CRL(crl);
				Handle<PkiItem> item = objectToPKIItem(hcrl);
				item->category = new std::string(*category);
				item->crl = hcrl;

				providerItemCollection->push(item);
			}
		} while (pCrlContext != NULL);
	}
	catch (Handle<Exception> &e){
		THROW_EXCEPTION(0, ProviderMicrosoft, e, "Error enum CRLs in store");
	}
}

Handle<PkiItem> ProviderMicrosoft::objectToPKIItem(Handle<Certificate> cert){
	LOGGER_FN();

	try{
		if (cert.isEmpty()){
			THROW_EXCEPTION(0, ProviderMicrosoft, NULL, "Certificate empty");
		}

		Handle<PkiItem> item = new PkiItem();

		item->format = new std::string("DER");
		item->type = new std::string("CERTIFICATE");
		item->provider = new std::string("MICROSOFT");

		char * hexHash;
		Handle<std::string> hhash = cert->getThumbprint();
		PkiStore::bin_to_strhex((unsigned char *)hhash->c_str(), hhash->length(), &hexHash);
		item->hash = new std::string(hexHash);

		item->certSubjectName = cert->getSubjectName();
		item->certSubjectFriendlyName = cert->getSubjectFriendlyName();
		item->certIssuerName = cert->getIssuerName();
		item->certIssuerFriendlyName = cert->getIssuerFriendlyName();
		item->certSerial = cert->getSerialNumber();
		item->certOrganizationName = cert->getOrganizationName();
		item->certSignatureAlgorithm = cert->getSignatureAlgorithm();
		item->certSignatureDigestAlgorithm = cert->getSignatureDigestAlgorithm();
		item->certPublicKeyAlgorithm = cert->getPublicKeyAlgorithm();

		item->certNotBefore = cert->getNotBefore();
		item->certNotAfter = cert->getNotAfter();

		return item;
	}
	catch (Handle<Exception> &e){
		THROW_EXCEPTION(0, ProviderMicrosoft, e, "Error create PkiItem from certificate");
	}
}

Handle<PkiItem> ProviderMicrosoft::objectToPKIItem(Handle<CRL> crl){
	LOGGER_FN();

	try{
		if (crl.isEmpty()){
			THROW_EXCEPTION(0, ProviderMicrosoft, NULL, "CRL empty");
		}

		Handle<PkiItem> item = new PkiItem();

		item->format = new std::string("DER");
		item->type = new std::string("CRL");
		item->provider = new std::string("MICROSOFT");

		char * hexHash;
		Handle<std::string> hhash = crl->getThumbprint();
		PkiStore::bin_to_strhex((unsigned char *)hhash->c_str(), hhash->length(), &hexHash);
		item->hash = new std::string(hexHash);

		item->crlIssuerName = crl->issuerName();
		item->crlIssuerFriendlyName = crl->issuerFriendlyName();
		item->crlLastUpdate = crl->getThisUpdate();
		item->crlNextUpdate = crl->getNextUpdate();
		item->crlSignatureAlgorithm = crl->getSignatureAlgorithm();
		item->crlSignatureDigestAlgorithm = crl->getSignatureDigestAlgorithm();
		item->crlAuthorityKeyid = crl->getAuthorityKeyid();
		item->crlCrlNumber = crl->getCrlNumber();

		return item;
	}
	catch (Handle<Exception> &e){
		THROW_EXCEPTION(0, ProviderMicrosoft, e, "Error create PkiItem from crl");
	}
}

Handle<Certificate> ProviderMicrosoft::getCert(Handle<std::string> hash, Handle<std::string> category){
	LOGGER_FN();

	X509 *hcert = NULL;

	try{
		HCERTSTORE hCertStore;
		PCCERT_CONTEXT pCertContext = NULL;

		const unsigned char *p;

		std::wstring wCategory = std::wstring(category->begin(), category->end());

		char cHash[20] = { 0 };
		hex2bin(hash->c_str(), cHash);

		CRYPT_HASH_BLOB cblobHash;
		cblobHash.pbData = (BYTE *)cHash;
		cblobHash.cbData = (DWORD)20;

		hCertStore = CertOpenStore(
			CERT_STORE_PROV_SYSTEM,
			PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
			NULL,
			CERT_SYSTEM_STORE_CURRENT_USER,
			wCategory.c_str()
			);

		if (!hCertStore) {
			THROW_EXCEPTION(0, ProviderMicrosoft, NULL, "Error open store");
		}

		pCertContext = CertFindCertificateInStore(
			hCertStore,
			X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
			0,
			CERT_FIND_HASH,
			&cblobHash,
			NULL);

		if (pCertContext) {
			p = pCertContext->pbCertEncoded;
			LOGGER_OPENSSL(d2i_X509);

			if (!(hcert = d2i_X509(NULL, &p, pCertContext->cbCertEncoded))) {
				THROW_OPENSSL_EXCEPTION(0, ProviderMicrosoft, NULL, "'d2i_X509' Error decode len bytes");
			}

			CertFreeCertificateContext(pCertContext);

			CertCloseStore(hCertStore, 0);

			return new Certificate(hcert);
		}
		else{
			THROW_EXCEPTION(0, ProviderMicrosoft, NULL, "Cannot find certificate in store");
		}
	}
	catch (Handle<Exception> &e){
		THROW_EXCEPTION(0, ProviderMicrosoft, e, "Error get certificate");
	}
}

Handle<CRL> ProviderMicrosoft::getCRL(Handle<std::string> hash, Handle<std::string> category){
	LOGGER_FN();

	try{
		HCERTSTORE hCertStore;
		PCCRL_CONTEXT pCrlContext = NULL;

		const unsigned char *p;

		std::wstring wCategory = std::wstring(category->begin(), category->end());

		hCertStore = CertOpenStore(
			CERT_STORE_PROV_SYSTEM,
			PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
			NULL,
			CERT_SYSTEM_STORE_CURRENT_USER,
			wCategory.c_str()
			);

		if (!hCertStore) {
			THROW_EXCEPTION(0, ProviderMicrosoft, NULL, "Error open store");
		}

		do
		{
			pCrlContext = CertEnumCRLsInStore(hCertStore, pCrlContext);
			if (pCrlContext){
				X509_CRL *tempCrl = NULL;
				p = pCrlContext->pbCrlEncoded;
				LOGGER_OPENSSL(d2i_X509_CRL);
				if (!(tempCrl = d2i_X509_CRL(NULL, &p, pCrlContext->cbCrlEncoded))) {
					THROW_OPENSSL_EXCEPTION(0, ProviderMicrosoft, NULL, "'d2i_X509_CRL' Error decode len bytes");
				}

				Handle<CRL> hTempCrl = new CRL(tempCrl);

				char * hexHash;
				Handle<std::string> hhash = hTempCrl->getThumbprint();
				PkiStore::bin_to_strhex((unsigned char *)hhash->c_str(), hhash->length(), &hexHash);
				std::string sh(hexHash);

				if (strcmp(sh.c_str(), hash->c_str()) == 0){
					return hTempCrl;
				}
			}
		} while (pCrlContext != NULL);

		CertCloseStore(hCertStore, 0);

		THROW_EXCEPTION(0, ProviderMicrosoft, NULL, "Cannot find CRL in store");
	}
	catch (Handle<Exception> &e){
		THROW_EXCEPTION(0, ProviderMicrosoft, e, "Error get CRL");
	}
}

Handle<Key> ProviderMicrosoft::getKey(Handle<Certificate> cert) {
	LOGGER_FN();

	EVP_PKEY_CTX *pctx = NULL;
	EVP_PKEY *pkey = NULL;
	EVP_MD_CTX *mctx = NULL;

	try{
		EVP_PKEY *pubkey;
		LOGGER_OPENSSL(X509_get_pubkey);
		pubkey = X509_get_pubkey(cert->internal());
		if (!pubkey) {
			THROW_OPENSSL_EXCEPTION(0, ProviderMicrosoft, NULL, "X509_get_pubkey");
		}

		if (pubkey->type == EVP_PKEY_RSA || pubkey->type == EVP_PKEY_DSA) {
#ifndef OPENSSL_NO_ENGINE
			if (!hasPrivateKey(cert)) {
				return NULL;
			}

			if (ENGINE *e = ENGINE_by_id("capi"))
			{
				if (ENGINE_init(e))
				{
					PCCERT_CONTEXT pCertContext = HCRYPT_NULL;
					PCCERT_CONTEXT pCertFound = HCRYPT_NULL;
					HCERTSTORE hCertStore = HCRYPT_NULL;

					pCertContext = Csp::createCertificateContext(cert);

					if (HCRYPT_NULL == (hCertStore = CertOpenStore(
						CERT_STORE_PROV_SYSTEM,
						X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
						HCRYPT_NULL,
						CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_READONLY_FLAG,
						L"My")))
					{
						THROW_EXCEPTION(0, ProviderMicrosoft, NULL, "CertOpenStore(My) failed");
					}

					if (!Csp::findExistingCertificate(pCertFound, hCertStore, pCertContext)) {
						THROW_EXCEPTION(0, ProviderMicrosoft, NULL, "findExistingCertificate");
					}

					CertFreeCertificateContext(pCertContext);
					pCertContext = HCRYPT_NULL;

					CertCloseStore(hCertStore, 0);
					hCertStore = HCRYPT_NULL;

					Handle<std::string> name = nameToStr(pCertFound->dwCertEncodingType, &pCertFound->pCertInfo->Subject);

					LOGGER_OPENSSL(ENGINE_load_private_key);
					pkey = ENGINE_load_private_key(e, name->c_str(), 0, 0);
					if (!pkey) {
						THROW_OPENSSL_EXCEPTION(0, ProviderMicrosoft, NULL, "ENGINE_load_private_key");
					}

					LOGGER_OPENSSL(ENGINE_free);
					ENGINE_free(e);
					e = NULL;
				}
				else {
					THROW_OPENSSL_EXCEPTION(0, ProviderMicrosoft, NULL, "Error init capi engine");
				}
			}
			else {
				THROW_OPENSSL_EXCEPTION(0, ProviderMicrosoft, NULL, "ENGINE 'capi' is not loaded");
			}

			return new Key(pkey);
#else
			THROW_EXCEPTION(0, ProviderMicrosoft, NULL, "Wrong Algorithm type");
#endif //OPENSSL_NO_ENGINE
		}

#ifndef OPENSSL_NO_CTGOSTCP
#define MAX_SIGNATURE_LEN 128
		ENGINE *e = ENGINE_by_id("ctgostcp");
		if (e == NULL) {
			THROW_OPENSSL_EXCEPTION(0, ProviderMicrosoft, NULL, "CTGSOTCP is not loaded");
		}

		LOGGER_OPENSSL(EVP_PKEY_CTX_new_id);
		pctx = EVP_PKEY_CTX_new_id(NID_id_GostR3410_2001, e);

		LOGGER_OPENSSL(EVP_PKEY_keygen_init);
		if (EVP_PKEY_keygen_init(pctx) <= 0){
			THROW_OPENSSL_EXCEPTION(0, ProviderMicrosoft, NULL, "EVP_PKEY_keygen_init");
		}

		LOGGER_OPENSSL(EVP_PKEY_CTX_ctrl_str);
		if (EVP_PKEY_CTX_ctrl_str(pctx, CTGOSTCP_PKEY_CTRL_STR_PARAM_KEYSET, "all") <= 0){
			THROW_OPENSSL_EXCEPTION(0, ProviderMicrosoft, NULL, "EVP_PKEY_CTX_ctrl_str CTGOSTCP_PKEY_CTRL_STR_PARAM_KEYSET 'all'");
		}

		LOGGER_OPENSSL(EVP_PKEY_CTX_ctrl_str);
		if (EVP_PKEY_CTX_ctrl_str(pctx, CTGOSTCP_PKEY_CTRL_STR_PARAM_EXISTING, "true") <= 0){
			THROW_OPENSSL_EXCEPTION(0, ProviderMicrosoft, NULL, "EVP_PKEY_CTX_ctrl_str CTGOSTCP_PKEY_CTRL_STR_PARAM_EXISTING 'true'");
		}

		LOGGER_OPENSSL(CTGOSTCP_EVP_PKEY_CTX_init_key_by_cert);
		if (CTGOSTCP_EVP_PKEY_CTX_init_key_by_cert(pctx, cert->internal()) <= 0){
			THROW_OPENSSL_EXCEPTION(0, ProviderMicrosoft, NULL, "Can not init key context by certificate");
		}

		LOGGER_OPENSSL(EVP_PKEY_CTX_ctrl_str);
		if (EVP_PKEY_CTX_ctrl_str(pctx, CTGOSTCP_PKEY_CTRL_STR_PARAM_EXISTING, "true") <= 0){
			THROW_OPENSSL_EXCEPTION(0, ProviderMicrosoft, NULL, "Parameter 'existing' setting error");
		}

		LOGGER_OPENSSL(EVP_PKEY_keygen);
		if (EVP_PKEY_keygen(pctx, &pkey) <= 0){
			THROW_OPENSSL_EXCEPTION(0, ProviderMicrosoft, NULL, "Can not init key by certificate");
		}

		int md_type = 0;
		const EVP_MD *md = NULL;

		LOGGER_OPENSSL(EVP_PKEY_get_default_digest_nid);
		if (EVP_PKEY_get_default_digest_nid(pkey, &md_type) <= 0) {
			THROW_OPENSSL_EXCEPTION(0, ProviderMicrosoft, NULL, "default digest for key type not found");
		}

		LOGGER_OPENSSL(EVP_get_digestbynid);
		md = EVP_get_digestbynid(md_type);

		LOGGER_OPENSSL(EVP_MD_CTX_create);
		if (!(mctx = EVP_MD_CTX_create())) {
			THROW_OPENSSL_EXCEPTION(0, ProviderMicrosoft, NULL, "Error creating digest context");
		}

		return new Key(pkey);
#endif //OPENSSL_NO_CTGOSTCP
	}
	catch (Handle<Exception> &e){
		THROW_EXCEPTION(0, ProviderMicrosoft, e, "Error get key");
	}

	EVP_MD_CTX_destroy(mctx);
	EVP_PKEY_free(pkey);
	EVP_PKEY_CTX_free(pctx);

	return NULL;
}

void ProviderMicrosoft::addPkiObject(Handle<Certificate> cert, Handle<std::string> category, Handle<std::string> contName, int provType){
	LOGGER_FN();

	PCCERT_CONTEXT pCertContext = HCRYPT_NULL;
	HCERTSTORE hCertStore = HCRYPT_NULL;
	HCRYPTPROV hProv = NULL;
	HCRYPTKEY hKey = NULL;

	try{
		DWORD dwKeySpec, dwSize;
		ALG_ID dwAlgId = 0;
		WCHAR wzContName[MAX_PATH];
		DWORD dwNewProvType = 0;
		CRYPT_KEY_PROV_INFO pKeyInfo = { 0 };

		std::wstring wCategory = std::wstring(category->begin(), category->end());

		pCertContext = Csp::createCertificateContext(cert);

		if (!contName.isEmpty() && provType) {
			if (!CryptAcquireContext(
				&hProv,
				contName->c_str(),
				NULL,
				provType,
				0))
			{
				THROW_EXCEPTION(0, ProviderMicrosoft, NULL, "CryptAcquireContext. Error: 0x%08x", GetLastError());
			}

			if (!CryptGetUserKey(hProv, AT_SIGNATURE, &hKey)) {
				CryptDestroyKey(hKey);

				if (!CryptGetUserKey(hProv, AT_KEYEXCHANGE, &hKey)) {
					THROW_EXCEPTION(0, Csp, NULL, "CryptGetUserKey. Error: 0x%08x", GetLastError());
				}
				else {
					dwKeySpec = AT_KEYEXCHANGE;
				}
			}
			else {
				dwKeySpec = AT_SIGNATURE;
			}

			if (mbstowcs(wzContName, contName->c_str(), MAX_PATH) <= 0) {
				THROW_EXCEPTION(0, Csp, NULL, "mbstowcs failed");
			}

			dwSize = sizeof(dwAlgId);
			if (!CryptGetKeyParam(hKey, KP_ALGID, (LPBYTE)&dwAlgId, &dwSize, 0)) {
				THROW_EXCEPTION(0, Csp, NULL, "CryptGetKeyParam. Error: 0x%08x", GetLastError());
			}

			switch (dwAlgId) {
			case CALG_GR3410EL:
			case CALG_DH_EL_SF:
				dwNewProvType = PROV_GOST_2001_DH;
				break;

#if defined(PROV_GOST_2012_256)
			case CALG_GR3410_12_256:
			case CALG_DH_GR3410_12_256_SF:
				dwNewProvType = PROV_GOST_2012_256;
				break;

			case CALG_GR3410_12_512:
			case CALG_DH_GR3410_12_512_SF:
				dwNewProvType = PROV_GOST_2012_512;
				break;
#endif // PROV_GOST_2012_256

#if defined(CALG_ECDSA) && defined(CALG_ECDH)
			case CALG_ECDSA:
			case CALG_ECDH:
				dwNewProvType = PROV_EC_ECDSA_FULL;
				break;
#endif // defined(CALG_ECDSA) && defined(CALG_ECDH)

#if defined(CALG_RSA_SIGN) && defined(CALG_RSA_KEYX)
			case CALG_RSA_SIGN:
			case CALG_RSA_KEYX:
				dwNewProvType = PROV_RSA_AES;
				break;
#endif // defined(CALG_ECDSA) && defined(CALG_ECDH)

			default:
				THROW_EXCEPTION(0, Csp, NULL, "Unsupported container key type", GetLastError());
				break;
			}

			pKeyInfo.dwKeySpec = dwKeySpec;
			pKeyInfo.dwProvType = dwNewProvType;
			pKeyInfo.pwszContainerName = wzContName;
			pKeyInfo.pwszProvName = (LPWSTR)Csp::provTypeToProvNameW(dwNewProvType);

			if (!CertSetCertificateContextProperty(
				pCertContext,
				CERT_KEY_PROV_INFO_PROP_ID,
				CERT_STORE_NO_CRYPT_RELEASE_FLAG,
				&pKeyInfo
				))
			{
				THROW_EXCEPTION(0, Csp, NULL, "CertSetCertificateContextProperty: Code: %d", GetLastError());
			};
		}

		if (HCRYPT_NULL == (hCertStore = CertOpenStore(
			CERT_STORE_PROV_SYSTEM,
			X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
			HCRYPT_NULL,
			CERT_SYSTEM_STORE_CURRENT_USER,
			wCategory.c_str())))
		{
			THROW_EXCEPTION(0, ProviderMicrosoft, NULL, "CertOpenStore failed: %s Code: %d", category->c_str(), GetLastError());
		}

		if (!CertAddCertificateContextToStore(
			hCertStore,
			pCertContext,
			CERT_STORE_ADD_REPLACE_EXISTING,
			NULL
			))
		{
			THROW_EXCEPTION(0, ProviderMicrosoft, NULL, "CertAddCertificateContextToStore failed. Code: %d", GetLastError())
		}

		CertCloseStore(hCertStore, 0);
		hCertStore = HCRYPT_NULL;

		if (pCertContext) {
			CertFreeCertificateContext(pCertContext);
			pCertContext = HCRYPT_NULL;
		}

		if (hKey) {
			CryptDestroyKey(hKey);
			hKey = NULL;
		}

		if (hProv) {
			if (!CryptReleaseContext(hProv, 0)) {
				THROW_EXCEPTION(0, ProviderMicrosoft, NULL, "CryptReleaseContext. Error: 0x%08x", GetLastError());
			}

			hProv = NULL;
		}
	}
	catch (Handle<Exception> &e){
		if (hCertStore) {
			CertCloseStore(hCertStore, 0);
			hCertStore = HCRYPT_NULL;
		}

		if (pCertContext) {
			CertFreeCertificateContext(pCertContext);
			pCertContext = HCRYPT_NULL;
		}

		if (hKey) {
			CryptDestroyKey(hKey);
			hKey = NULL;
		}

		if (hProv) {
			if (!CryptReleaseContext(hProv, 0)) {
				THROW_EXCEPTION(0, ProviderMicrosoft, NULL, "CryptReleaseContext. Error: 0x%08x", GetLastError());
			}

			hProv = NULL;
		}

		THROW_EXCEPTION(0, ProviderMicrosoft, e, "Error add certificate to store");
	}
}

void ProviderMicrosoft::addPkiObject(Handle<CRL> crl, Handle<std::string> category){
	LOGGER_FN();

	PCCRL_CONTEXT pCrlContext = HCRYPT_NULL;
	HCERTSTORE hCertStore = HCRYPT_NULL;
	HCRYPTPROV hProv = NULL;

	try{
		std::wstring wCategory = std::wstring(category->begin(), category->end());

		pCrlContext = Csp::createCrlContext(crl);

		if (HCRYPT_NULL == (hCertStore = CertOpenStore(
			CERT_STORE_PROV_SYSTEM,
			X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
			HCRYPT_NULL,
			CERT_SYSTEM_STORE_CURRENT_USER,
			wCategory.c_str())))
		{
			THROW_EXCEPTION(0, ProviderMicrosoft, NULL, "CertOpenStore failed: %s Code: %d", category->c_str(), GetLastError());
		}

		if (!CertAddCRLContextToStore(
			hCertStore,
			pCrlContext,
			CERT_STORE_ADD_REPLACE_EXISTING,
			NULL
			))
		{
			THROW_EXCEPTION(0, ProviderMicrosoft, NULL, "CertAddCertificateContextToStore failed. Code: %d", GetLastError())
		}

		CertCloseStore(hCertStore, 0);
		hCertStore = HCRYPT_NULL;

		if (pCrlContext) {
			CertFreeCRLContext(pCrlContext);
			pCrlContext = HCRYPT_NULL;
		}
	}
	catch (Handle<Exception> &e){
		if (hCertStore) {
			CertCloseStore(hCertStore, 0);
			hCertStore = HCRYPT_NULL;
		}

		if (pCrlContext) {
			CertFreeCRLContext(pCrlContext);
			pCrlContext = HCRYPT_NULL;
		}

		THROW_EXCEPTION(0, ProviderMicrosoft, e, "Error add certificate to store");
	}
}

void ProviderMicrosoft::deletePkiObject(Handle<Certificate> cert, Handle<std::string> category){
	LOGGER_FN();

	PCCERT_CONTEXT pCertContext = HCRYPT_NULL;
	PCCERT_CONTEXT pCertFound = HCRYPT_NULL;
	HCERTSTORE hCertStore = HCRYPT_NULL;

	try{
		std::wstring wCategory = std::wstring(category->begin(), category->end());

		pCertContext = Csp::createCertificateContext(cert);

		if (HCRYPT_NULL == (hCertStore = CertOpenStore(
			CERT_STORE_PROV_SYSTEM,
			X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
			HCRYPT_NULL,
			CERT_SYSTEM_STORE_CURRENT_USER,
			wCategory.c_str())))
		{
			THROW_EXCEPTION(0, ProviderMicrosoft, NULL, "CertOpenStore failed: %s Code: %d", category->c_str(), GetLastError());
		}

		if (!Csp::findExistingCertificate(pCertFound, hCertStore, pCertContext)) {
			THROW_EXCEPTION(0, ProviderMicrosoft, NULL, "Cannot find existing certificate");
		}

		if (!CertDeleteCertificateFromStore(pCertFound)) {
			THROW_EXCEPTION(0, ProviderMicrosoft, NULL, "CertDeleteCertificateFromStore failed: Code: %d", GetLastError());
		}

		if (hCertStore) {
			CertCloseStore(hCertStore, 0);
			hCertStore = HCRYPT_NULL;
		}

		if (pCertContext) {
			CertFreeCertificateContext(pCertContext);
			pCertContext = HCRYPT_NULL;
		}

		if (pCertFound) {
			CertFreeCertificateContext(pCertFound);
			pCertFound = HCRYPT_NULL;
		}
	}
	catch (Handle<Exception> &e){
		if (hCertStore) {
			CertCloseStore(hCertStore, 0);
			hCertStore = HCRYPT_NULL;
		}

		if (pCertContext) {
			CertFreeCertificateContext(pCertContext);
			pCertContext = HCRYPT_NULL;
		}

		if (pCertFound) {
			CertFreeCertificateContext(pCertFound);
			pCertFound = HCRYPT_NULL;
		}

		THROW_EXCEPTION(0, ProviderMicrosoft, e, "Error add certificate to store");
	}
}

void ProviderMicrosoft::deletePkiObject(Handle<CRL> crl, Handle<std::string> category){
	LOGGER_FN();

	PCCRL_CONTEXT pCrlContext = HCRYPT_NULL;
	PCCRL_CONTEXT pCrlFound = HCRYPT_NULL;
	HCERTSTORE hCertStore = HCRYPT_NULL;

	try{
		std::wstring wCategory = std::wstring(category->begin(), category->end());

		pCrlContext = Csp::createCrlContext(crl);

		if (HCRYPT_NULL == (hCertStore = CertOpenStore(
			CERT_STORE_PROV_SYSTEM,
			X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
			HCRYPT_NULL,
			CERT_SYSTEM_STORE_CURRENT_USER,
			wCategory.c_str())))
		{
			THROW_EXCEPTION(0, ProviderMicrosoft, NULL, "CertOpenStore failed: %s Code: %d", category->c_str(), GetLastError());
		}

		if (!Csp::findExistingCrl(pCrlFound, hCertStore, pCrlContext)) {
			THROW_EXCEPTION(0, ProviderMicrosoft, NULL, "Cannot find existing certificate");
		}

		if (!CertDeleteCRLFromStore(pCrlFound)) {
			THROW_EXCEPTION(0, ProviderMicrosoft, NULL, "CertDeleteCRLFromStore failed: Code: %d", GetLastError());
		}

		if (hCertStore) {
			CertCloseStore(hCertStore, 0);
			hCertStore = HCRYPT_NULL;
		}

		if (pCrlContext) {
			CertFreeCRLContext(pCrlContext);
			pCrlContext = HCRYPT_NULL;
		}

		if (pCrlFound) {
			CertFreeCRLContext(pCrlFound);
			pCrlFound = HCRYPT_NULL;
		}
	}
	catch (Handle<Exception> &e){
		if (hCertStore) {
			CertCloseStore(hCertStore, 0);
			hCertStore = HCRYPT_NULL;
		}

		if (pCrlContext) {
			CertFreeCRLContext(pCrlContext);
			pCrlContext = HCRYPT_NULL;
		}

		if (pCrlFound) {
			CertFreeCRLContext(pCrlFound);
			pCrlFound = HCRYPT_NULL;
		}

		THROW_EXCEPTION(0, ProviderMicrosoft, e, "Error add CRL to store");
	}
}

bool ProviderMicrosoft::hasPrivateKey(Handle<Certificate> cert) {
	LOGGER_FN();

	try {
		PCCERT_CONTEXT pCertContext = HCRYPT_NULL;
		PCCERT_CONTEXT pCertFound = HCRYPT_NULL;
		HCERTSTORE hCertStore = HCRYPT_NULL;
		DWORD dwSize = 0;
		bool res = false;

		pCertContext = Csp::createCertificateContext(cert);

		if (HCRYPT_NULL == (hCertStore = CertOpenStore(
			CERT_STORE_PROV_SYSTEM,
			X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
			HCRYPT_NULL,
			CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_READONLY_FLAG,
			L"My")))
		{
			THROW_EXCEPTION(0, ProviderMicrosoft, NULL, "CertOpenStore(My) failed");
		}

		if (Csp::findExistingCertificate(pCertFound, hCertStore, pCertContext)) {
			if (CertGetCertificateContextProperty(pCertFound, CERT_KEY_PROV_INFO_PROP_ID, NULL, &dwSize)) {
				res = true;
			}

			CertFreeCertificateContext(pCertFound);
			pCertFound = HCRYPT_NULL;
		}

		CertFreeCertificateContext(pCertContext);
		pCertContext = HCRYPT_NULL;

		CertCloseStore(hCertStore, 0);
		hCertStore = HCRYPT_NULL;

		return res;
	}
	catch (Handle<Exception> &e) {
		THROW_EXCEPTION(0, ProviderMicrosoft, e, "Error check key existing");
	}
}

bool ProviderMicrosoft::hasPrivateKey(PCCERT_CONTEXT pCertContext) {
	LOGGER_FN();

	try {
		DWORD dwSize = 0;
		bool res = false;

		if (CertGetCertificateContextProperty(pCertContext, CERT_KEY_PROV_INFO_PROP_ID, NULL, &dwSize)) {
			res = true;
		}

		return res;
	}
	catch (Handle<Exception> &e) {
		THROW_EXCEPTION(0, ProviderMicrosoft, e, "Error check key existing");
	}
}

Handle<std::string> ProviderMicrosoft::nameToStr(
	IN DWORD dwCertEncodingType,
	IN const CERT_NAME_BLOB *pName,
	IN DWORD dwStrType
	)
{
	LOGGER_FN();

	try {
		LPTSTR pszString;
		DWORD cbSize;
		Handle<std::string> res;

		cbSize = CertNameToStr(
			dwCertEncodingType,
			const_cast<PCERT_NAME_BLOB> (pName),
			dwStrType,
			NULL,
			0);

		if (!cbSize) {
			THROW_EXCEPTION(0, ProviderMicrosoft, NULL, "CertNameToStr. Code: %d", GetLastError());
		}

		if (!(pszString = (LPTSTR)malloc(cbSize * sizeof(TCHAR)))) {
			THROW_EXCEPTION(0, ProviderMicrosoft, NULL, "Memory allocation failed");
		}

		cbSize = CertNameToStr(
			dwCertEncodingType,
			const_cast<PCERT_NAME_BLOB> (pName),
			dwStrType,
			pszString,
			cbSize);

		res = new std::string(pszString);

		free(pszString);
		pszString = NULL;

		return res;
	}
	catch (Handle<Exception> &e) {
		THROW_EXCEPTION(0, ProviderMicrosoft, e, "Error convert name to string");
	}
}

int ProviderMicrosoft::char2int(char input) {
	LOGGER_FN();

	try{
		if (input >= '0' && input <= '9'){
			return input - '0';
		}

		if (input >= 'A' && input <= 'F'){
			return input - 'A' + 10;
		}

		if (input >= 'a' && input <= 'f'){
			return input - 'a' + 10;
		}

		THROW_EXCEPTION(0, ProviderMicrosoft, NULL, "Invalid input string");
	}
	catch (Handle<Exception> &e){
		THROW_EXCEPTION(0, ProviderMicrosoft, e, "Error char to int");
	}

}

void ProviderMicrosoft::hex2bin(const char* src, char* target) {
	LOGGER_FN();

	while (*src && src[1]){
		*(target++) = char2int(*src) * 16 + char2int(src[1]);
		src += 2;
	}
}
