#include "../stdafx.h"

#include "signed_data.h"

IMPLEMENT_PEM_write(CMS, CMS_ContentInfo, PEM_STRING_CMS, CMS_ContentInfo)

Handle<CertificateCollection> SignedData::certificates(){
	LOGGER_FN();

	STACK_OF(X509) *certs;

	LOGGER_OPENSSL(CMS_get1_certs);
	certs = CMS_get1_certs(this->internal());

	return new CertificateCollection(certs);
}

Handle<Certificate> SignedData::certificates(int index){
	LOGGER_FN();

	return this->certificates()->items(index);
}

Handle<SignerCollection> SignedData::signers(){
	LOGGER_FN();

	stack_st_CMS_SignerInfo *signers_ = CMS_get0_SignerInfos(this->internal());

	return new SignerCollection(signers_, this->handle());
}

Handle<Signer> SignedData::signers(int index){
	LOGGER_FN();

	return this->signers()->items(index);
}

bool SignedData::isDetached(){
	LOGGER_FN();

	LOGGER_OPENSSL("CMS_is_detached");
	int res = CMS_is_detached(this->internal());

	if (res == -1){
		THROW_OPENSSL_EXCEPTION(0, SignedData, NULL, "CMS_is_detached");
	}

	return res == 1;
}

void SignedData::read(Handle<Bio> in, DataFormat::DATA_FORMAT format){
	if (in.isEmpty())
		THROW_EXCEPTION(0, SignedData, NULL, "Parameter %d cann't be NULL", 1);

	CMS_ContentInfo *ci = NULL;

	in->reset();

	switch (format){
	case DataFormat::DER:
		LOGGER_OPENSSL("d2i_CMS_bio");
		ci = d2i_CMS_bio(in->internal(), NULL);

		if (ci && !CMS_is_detached(ci)){
			LOGGER_INFO("Get content from DER file");

			ASN1_OCTET_STRING *asn = (*CMS_get0_content(ci));
			Handle<Bio> content = new Bio(BIO_new_mem_buf(asn->data, asn->length));

			this->setContent(content);
		}
		break;
	case DataFormat::BASE64:
	{
		BIO *content_ = this->content->internal();
		LOGGER_OPENSSL("SMIME_read_CMS");
		ci = SMIME_read_CMS(in->internal(), &content_);
		break;
	}
	default:
		THROW_EXCEPTION(0, SignedData, NULL, ERROR_DATA_FORMAT_UNKNOWN_FORMAT, format);
	}

	if (!ci) {
		puts(OpenSSL::printErrors()->c_str());
		THROW_EXCEPTION(0, SignedData, NULL, "Can not read CMS signed data from BIO");
	}

	this->setData(ci);
}

void SignedData::write(Handle<Bio> out, DataFormat::DATA_FORMAT format){
	LOGGER_FN();

	if (out.isEmpty())
		THROW_EXCEPTION(0, SignedData, NULL, "Parameter %d is NULL", 1);

	switch (format){
	case DataFormat::DER:

		LOGGER_OPENSSL("i2d_CMS_bio");
		if (i2d_CMS_bio(out->internal(), this->internal()) < 1)
			THROW_OPENSSL_EXCEPTION(0, SignedData, NULL, "i2d_CMS_bio", NULL);

		// LOGGER_OPENSSL("i2d_CMS_bio_stream");
		// if (i2d_CMS_bio_stream(out->internal(), this->internal(), this->content->internal(), flags) < 1)
		//   THROW_OPENSSL_EXCEPTION(0, SignedData, NULL, "i2d_CMS_bio_stream", NULL);

		break;
	case DataFormat::BASE64:
		LOGGER_OPENSSL("PEM_write_bio_CMS");
		if (PEM_write_bio_CMS(out->internal(), this->internal()) < 1)
			THROW_OPENSSL_EXCEPTION(0, SignedData, NULL, "PEM_write_bio_CMS", NULL);

		// LOGGER_OPENSSL("PEM_write_bio_CMS_stream");
		// if (PEM_write_bio_CMS_stream(out->internal(), this->internal(), this->content->internal(), flags) < 1)
		//   THROW_OPENSSL_EXCEPTION(0, SignedData, NULL, "PEM_write_bio_CMS_stream", NULL);

		break;

	default:
		THROW_EXCEPTION(0, SignedData, NULL, ERROR_DATA_FORMAT_UNKNOWN_FORMAT, format);
	}
}

Handle<Signer> SignedData::createSigner(Handle<Certificate> cert, Handle<Key> pkey, Handle<std::string> digestname){
	LOGGER_FN();

	const EVP_MD* md = EVP_get_digestbyname(digestname->c_str());
	if (!md){
		THROW_OPENSSL_EXCEPTION(0, SignedData, NULL, "Unknown digest name '%s'", digestname->c_str());
	}

	LOGGER_OPENSSL("CMS_add1_signer");
	CMS_SignerInfo *signer = CMS_add1_signer(this->internal(), cert->internal(), pkey->internal(), md, flags);
	if (!signer){
		THROW_OPENSSL_EXCEPTION(0, SignedData, NULL, "CMS_add1_signer");
	}

	return new Signer(signer, this->handle());
}

void SignedData::addCertificate(Handle<Certificate> cert){
	LOGGER_FN();

	Handle<Certificate> cert_copy = cert->duplicate();

	LOGGER_OPENSSL("CMS_add0_cert");
	if (CMS_add0_cert(this->internal(), cert_copy->internal()) < 1){
		THROW_EXCEPTION(0, SignedData, NULL, "Certificate already present");
	}

	cert_copy->setParent(this->handle());
}

void SignedData::setContent(Handle<Bio> value){
	LOGGER_FN();

	this->content = value;
}

Handle<Bio> SignedData::getContent(){
	LOGGER_FN();

	return this->content;
}

bool SignedData::verify(Handle<CertificateCollection> certs){
	LOGGER_FN();

	stack_st_X509 *pCerts = NULL;
	if (!certs.isEmpty()){
		pCerts = certs->internal();
	}

	// ����� ������� �� ������
	content->reset();

	X509_STORE *store = X509_STORE_new();

	LOGGER_OPENSSL("CMS_verify");
	int res = CMS_verify(this->internal(), pCerts, store, content->internal(), NULL, flags);
	X509_STORE_free(store);
	if (!res){
		THROW_OPENSSL_EXCEPTION(0, SignedData, NULL, "CMS_verify");
	}

	return res == 1;
}

Handle<SignedData> SignedData::sign(Handle<Certificate> cert, Handle<Key> pkey, Handle<CertificateCollection> certs, Handle<Bio> content, unsigned int flags){
	LOGGER_FN();

	LOGGER_OPENSSL("CMS_sign");
	CMS_ContentInfo *res = CMS_sign(cert->internal(), pkey->internal(), certs->internal(), content->internal(), flags);
	if (!res){
		THROW_OPENSSL_EXCEPTION(0, SignedData, NULL, "CMS_sign");
	}

	Handle<SignedData> sd = new SignedData(res);
	sd->flags = flags;

	return sd;
}

void SignedData::sign(){
	LOGGER_FN();

	if (!(flags & CMS_DETACHED)){
		CMS_set_detached(this->internal(), 0);
	}

	LOGGER_OPENSSL("CMS_final");
	if (CMS_final(this->internal(), this->content->internal(), NULL, flags) < 1){
		THROW_OPENSSL_EXCEPTION(0, SignedData, NULL, "CMS_final");
	}
}

int SignedData::getFlags(){
	LOGGER_FN();

	return this->flags;
}

void SignedData::setFlags(int v){
	LOGGER_FN();

	this->flags = v;
}

void SignedData::addFlag(int v){
	LOGGER_FN();

	this->flags |= v;
}
void SignedData::removeFlags(int v){
	LOGGER_FN();

	this->flags ^= v;
}