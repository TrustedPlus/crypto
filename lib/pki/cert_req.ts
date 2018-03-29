/// <reference path="../native.ts" />
/// <reference path="../object.ts" />

namespace trusted.pki {

    const DEFAULT_DATA_FORMAT: DataFormat = DataFormat.PEM;

    /**
     * Wrap X509_REQ
     *
     * @export
     * @class CertificationRequest
     * @extends {BaseObject<native.PKI.CertificationRequest>}
     */
    export class CertificationRequest extends BaseObject<native.PKI.CertificationRequest> {

        /**
         * Load request from file
         *
         * @static
         * @param {string} filename File location
         * @param {DataFormat} [format] PEM | DER
         *
         * @memberOf CertificationRequest
         */
        public static load(filename: string, format?: DataFormat): CertificationRequest {
            const req: CertificationRequest = new CertificationRequest();
            req.handle.load(filename, format);
            return req;
        }

        /**
         * Creates an instance of CertificationRequest.
         * @param {native.PKI.CertificationRequest} [param]
         *
         * @memberOf CertificationRequest
         */
        constructor(param?: native.PKI.CertificationRequest) {
            super();
            if (param instanceof native.PKI.CertificationRequestInfo) {
                this.handle = new native.PKI.CertificationRequest(param);
            } else {
                this.handle = new native.PKI.CertificationRequest();
            }
        }

        /**
         * Load request from file
         *
         * @param {string} filename File location
         * @param {DataFormat} [format] PEM | DER
         *
         * @memberOf CertificationRequest
         */
        public load(filename: string, format?: DataFormat): void {
            this.handle.load(filename, format);
        }

        /**
         * Write request to file
         *
         * @param {string} filename File path
         * @param {DataFormat} [dataFormat=DEFAULT_DATA_FORMAT]
         *
         * @memberOf CertificationRequest
         */
        public save(filename: string, dataFormat: DataFormat = DEFAULT_DATA_FORMAT): void {
            this.handle.save(filename, dataFormat);
        }

        /**
         * Rerutn subject name
         *
         * @readonly
         * @type {string}
         * @memberof CertificationRequest
         */
        get subject(): string {
            return this.handle.getSubject();
        }

        /**
         * Sets the subject of this certification request.
         *
         * @param {string} x509name Example "/C=US/O=Test/CN=example.com"
         *
         * @memberOf CertificationRequest
         */
        set subject(x509name: string) {
            this.handle.setSubject(x509name);
        }

        /**
         * Rerutn subject public key
         *
         * @readonly
         * @type {Key}
         * @memberof CertificationRequest
         */
        get publicKey(): Key {
            return Key.wrap<native.PKI.Key, Key>(this.handle.getPublicKey());
        }

        /**
         *  Set public key
         *
         *  @param {Key} pubkey Public key
         *
         * @memberOf CertificationRequest
         */
        set publicKey(pubkey: pki.Key) {
            this.handle.setPublicKey(pubkey.handle);
        }

        /**
         * Rerutn version
         *
         * @readonly
         * @type {number}
         * @memberof CertificationRequest
         */
        get version(): number {
            return this.handle.getVersion();
        }

        /**
         * Set version certificate
         *
         * @param {number} version
         *
         * @memberOf CertificationRequest
         */
        set version(version: number) {
            this.handle.setVersion(version);
        }

        /**
         *  Signs request using the given private key
         *
         * @param {Key} key private key to sign
         * @param {string} [digest="SHA1"] message digest to use (defaults sha1)
         * @memberof CertificationRequest
         */
        public sign(key: Key, digest: string = "SHA1"): void {
            this.handle.sign(key.handle, digest);
        }

        /**
         * Verify request
         *
         * @returns {boolean}
         *
         * @memberOf CertificationRequest
         */
        public verify(): boolean {
            return this.handle.verify();
        }

        /**
         * Return request in PEM format
         *
         * @readonly
         * @type {Buffer}
         * @memberOf CertificationRequest
         */
        get PEMString(): Buffer {
            return this.handle.getPEMString();
        }
    }

}