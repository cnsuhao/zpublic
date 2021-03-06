//
// FTPStreamFactory.cpp
//
// $Id: //poco/1.4/Net/src/FTPStreamFactory.cpp#2 $
//
// Library: Net
// Package: FTP
// Module:  FTPStreamFactory
//
// Copyright (c) 2005-2006, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
// 
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//


#include "Poco/Net/FTPStreamFactory.h"
#include "Poco/Net/FTPClientSession.h"
#include "Poco/Net/NetException.h"
#include "Poco/URI.h"
#include "Poco/URIStreamOpener.h"
#include "Poco/UnbufferedStreamBuf.h"
#include "Poco/Path.h"


using Poco::URIStreamFactory;
using Poco::URI;
using Poco::URIStreamOpener;
using Poco::UnbufferedStreamBuf;
using Poco::Path;


namespace Poco {
namespace Net {


class FTPStreamBuf: public UnbufferedStreamBuf
{
public:
	FTPStreamBuf(std::istream& istr):
		_istr(istr)
	{
	}
	
	~FTPStreamBuf()
	{
	}
		
private:
	int readFromDevice()
	{
		return _istr.get();
	}
	
	std::istream& _istr;
};


class FTPIOS: public virtual std::ios
{
public:
	FTPIOS(std::istream& istr):
		_buf(istr)
	{
		poco_ios_init(&_buf);
	}
	
	~FTPIOS()
	{
	}
	
	FTPStreamBuf* rdbuf()
	{
		return &_buf;
	}

protected:
	FTPStreamBuf _buf;
};


class FTPStream: public FTPIOS, public std::istream
{
public:
	FTPStream(std::istream& istr, FTPClientSession* pSession):
		FTPIOS(istr),
		std::istream(&_buf),
		_pSession(pSession)
	{
	}
		
	~FTPStream()
	{
		delete _pSession;
	}
	
private:
	FTPClientSession* _pSession;
};


FTPPasswordProvider::FTPPasswordProvider()
{
}


FTPPasswordProvider::~FTPPasswordProvider()
{
}


std::string          FTPStreamFactory::_anonymousPassword("poco@localhost");
FTPPasswordProvider* FTPStreamFactory::_pPasswordProvider(0);


FTPStreamFactory::FTPStreamFactory()
{
}


FTPStreamFactory::~FTPStreamFactory()
{
}


std::istream* FTPStreamFactory::open(const URI& uri)
{
	poco_assert (uri.getScheme() == "ftp");

	FTPClientSession* pSession = new FTPClientSession(uri.getHost(), uri.getPort());
	try
	{
		std::string username;
		std::string password;
		getUserInfo(uri, username, password);
		
		std::string path;
		char        type;
		getPathAndType(uri, path, type);
			
		pSession->login(username, password);
		if (type == 'a')
			pSession->setFileType(FTPClientSession::TYPE_TEXT);
			
		Path p(path, Path::PATH_UNIX);
		p.makeFile();
		for (int i = 0; i < p.depth(); ++i)
			pSession->setWorkingDirectory(p[i]);
		std::string file(p.getFileName());
		std::istream& istr = (type == 'd' ? pSession->beginList(file) : pSession->beginDownload(file));
		return new FTPStream(istr, pSession);
	}
	catch (...)
	{
		delete pSession;
		throw;
	}
}


void FTPStreamFactory::setAnonymousPassword(const std::string& password)
{
	_anonymousPassword = password;
}

	
const std::string& FTPStreamFactory::getAnonymousPassword()
{
	return _anonymousPassword;
}

	
void FTPStreamFactory::setPasswordProvider(FTPPasswordProvider* pProvider)
{
	_pPasswordProvider = pProvider;
}

	
FTPPasswordProvider* FTPStreamFactory::getPasswordProvider()
{
	return _pPasswordProvider;
}


void FTPStreamFactory::splitUserInfo(const std::string& userInfo, std::string& username, std::string& password)
{
	std::string::size_type pos = userInfo.find(':');
	if (pos != std::string::npos)
	{
		username.assign(userInfo, 0, pos++);
		password.assign(userInfo, pos, userInfo.size() - pos);
	}
	else username = userInfo;
}


void FTPStreamFactory::getUserInfo(const URI& uri, std::string& username, std::string& password)
{
	splitUserInfo(uri.getUserInfo(), username, password);
	if (username.empty())
	{
		username = "anonymous";
		password = _anonymousPassword;
	}
	else if (password.empty())
	{
		if (_pPasswordProvider)
			password = _pPasswordProvider->password(username, uri.getHost());
		else
			throw FTPException(std::string("Password required for ") + username + "@" + uri.getHost());
	}
}


void FTPStreamFactory::getPathAndType(const Poco::URI& uri, std::string& path, char& type)
{
	path = uri.getPath();
	type = 'i';
	std::string::size_type pos = path.rfind(';');
	if (pos != std::string::npos)
	{
		if (path.length() == pos + 7 && path.compare(pos + 1, 5, "type=") == 0)
		{
			type = path[pos + 6];
			path.resize(pos);
		}
	}
}


void FTPStreamFactory::registerFactory()
{
	URIStreamOpener::defaultOpener().registerStreamFactory("ftp", new FTPStreamFactory);
}


void FTPStreamFactory::unregisterFactory()
{
	URIStreamOpener::defaultOpener().unregisterStreamFactory("ftp");
}


} } // namespace Poco::Net
