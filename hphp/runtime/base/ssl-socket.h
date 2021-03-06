/*
   +----------------------------------------------------------------------+
   | HipHop for PHP                                                       |
   +----------------------------------------------------------------------+
   | Copyright (c) 2010-2014 Facebook, Inc. (http://www.facebook.com)     |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
*/

#ifndef incl_HPHP_SSL_SOCKET_H_
#define incl_HPHP_SSL_SOCKET_H_

#include "hphp/runtime/base/socket.h"
#include "hphp/util/lock.h"
#include "hphp/util/network.h"
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/err.h>

namespace HPHP {
///////////////////////////////////////////////////////////////////////////////

/**
 * TCP sockets running SSL protocol.
 */
struct SSLSocket : Socket {
  enum class CryptoMethod {
    ClientSSLv2,
    ClientSSLv3,
    ClientSSLv23,
    ClientTLS,

    ServerSSLv2,
    ServerSSLv3,
    ServerSSLv23,
    ServerTLS,
  };

  static int GetSSLExDataIndex();
  static SSLSocket *Create(int fd, int domain, const HostURL &hosturl,
                           double timeout);

  SSLSocket();
  SSLSocket(int sockfd, int type, const char *address = nullptr, int port = 0);
  virtual ~SSLSocket();
  void sweep() override;

  // will setup and enable crypto
  bool onConnect();
  bool onAccept();

  CLASSNAME_IS("SSLSocket")
  // overriding ResourceData
  const String& o_getClassNameHook() const { return classnameof(); }

  // overriding Socket
  virtual bool close();
  virtual int64_t readImpl(char *buffer, int64_t length);
  virtual int64_t writeImpl(const char *buffer, int64_t length);
  virtual bool checkLiveness();

  Array &getContext() { return m_context;}

private:
  bool closeImpl();
  bool handleError(int64_t nr_bytes, bool is_init);

  bool setupCrypto(SSLSocket *session = nullptr);
  bool enableCrypto(bool activate = true);

  SSL *createSSL(SSL_CTX *ctx);
  bool applyVerificationPolicy(X509 *peer);

private:
  bool m_ssl_active{false};
  bool m_client{false};
  bool m_enable_on_connect{false};
  bool m_state_set{false};
  bool m_is_blocked{true};
  Array m_context;
  SSL *m_handle{nullptr};
  CryptoMethod m_method{(CryptoMethod)-1};
  double m_connect_timeout{0};

private:
  static Mutex s_mutex;
  static int s_ex_data_index;
};

///////////////////////////////////////////////////////////////////////////////
// helper class

class Certificate : public SweepableResourceData {
public:
  X509 *m_cert;
  explicit Certificate(X509 *cert) : m_cert(cert) { assert(m_cert);}
  ~Certificate() {
    if (m_cert) X509_free(m_cert);
  }

  X509* get() {
    return m_cert;
  }

  CLASSNAME_IS("OpenSSL X.509")
  // overriding ResourceData
  const String& o_getClassNameHook() const { return classnameof(); }

  DECLARE_RESOURCE_ALLOCATION(Certificate)

  /**
   * Given a variant, coerce it into an X509 object. It can be:
   *
   *  . X509 resource created using openssl_read_x509()
   *  . if it starts with file:// then it will be interpreted as the path
   *    to that cert
   *  . it will be interpreted as the cert data
   */
  static Resource Get(const Variant& var);
  static BIO *ReadData(const Variant& var, bool *file = nullptr);
};

///////////////////////////////////////////////////////////////////////////////
}

#endif // incl_HPHP_SSL_SOCKET_H_
