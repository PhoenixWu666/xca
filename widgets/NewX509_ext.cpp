/*
 * Copyright (C) 2001 Christian Hohnstaedt.
 *
 *  All rights reserved.
 *
 *
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions are met:
 *
 *  - Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  - Neither the name of the author nor the names of its contributors may be 
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * This program links to software with different licenses from:
 *
 *	http://www.openssl.org which includes cryptographic software
 * 	written by Eric Young (eay@cryptsoft.com)"
 *
 *	http://www.sleepycat.com
 *
 *	http://www.trolltech.com
 * 
 *
 *
 * http://www.hohnstaedt.de/xca
 * email: christian@hohnstaedt.de
 *
 * $Id$ 
 *
 */                           


#include "NewX509.h"
#include <qgroupbox.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qradiobutton.h>
#include <qlineedit.h>
#include <qwhatsthis.h>
#include <qlistbox.h>
#include <qlistview.h>
#include "MainWindow.h"


x509v3ext NewX509::getBasicConstraints()
{
	QStringList cont;
	x509v3ext ext;
	QString ca[] = { "", "CA:TRUE", "CA:FALSE" };
	if (basicCA->currentItem() > 0) {
		if (bcCritical->isChecked())
			cont << "critical";
		cont << ca[basicCA->currentItem()];
		if (!basicPath->text().isEmpty())
			cont << (QString)"pathlen:" + basicPath->text();
		ext.create(NID_basic_constraints, cont.join(", "));
	}
	return ext;
}

x509v3ext NewX509::getSubKeyIdent()
{
	x509v3ext ext;
	if (subKey->isChecked())
		ext.create(NID_subject_key_identifier, "hash");
	return ext;
}

x509v3ext NewX509::getAuthKeyIdent()
{
	x509v3ext ext;
	if (subKey->isChecked())
		ext.create(NID_authority_key_identifier, 
			"keyid:always,issuer:always", &ext_ctx);
	return ext;
}

x509v3ext NewX509::getKeyUsage()
{
	QString keyusage[] = {
		"digitalSignature", "nonRepudiation", "keyEncipherment",
		"dataEncipherment", "keyAgreement", "keyCertSign",
		"cRLSign", "encipherOnly", "decipherOnly"
	};
						
	QStringList cont;
	x509v3ext ext;
	QListBoxItem *item;
	if (kuCritical->isChecked()) cont << "critical";
        for (int i=0; (item = keyUsage->item(i)); i++) {
		if (item->selected()) {
			cont << keyusage[i];
		}
	}
	ext.create(NID_key_usage, cont.join(", "));
	return ext;
}

x509v3ext NewX509::getEkeyUsage()
{
	QStringList cont;
	x509v3ext ext;
	QListBoxItem *item;
	if (ekuCritical->isChecked()) cont << "critical";
	for (int i=0; (item = ekeyUsage->item(i)); i++) {
		if (item->selected()){
			cont << (QString)OBJ_nid2sn(eku_nid[i]);
		}
	}
	ext.create(NID_ext_key_usage, cont.join(", "));
	return ext;
}

x509v3ext NewX509::getSubAltName()
{
	QStringList cont;
	x509v3ext ext;
	if (subAltCp->isChecked() && subAltCp->isEnabled())
		cont << (QString)"email:" + emailAddress->text();
	cont << subAltName->text();
	ext.create(NID_subject_alt_name, cont.join(", "));
	return ext;
}

x509v3ext NewX509::getIssAltName()
{
	QStringList cont;
	x509v3ext ext;
	if (issAltCp->isChecked() && issAltCp->isEnabled())
		cont << (QString)"issuer:copy";
	cont << subAltName->text();
	ext.create(NID_subject_alt_name, cont.join(", "), &ext_ctx);
	return ext;
}

x509v3ext NewX509::getCrlDist()
{
	QStringList cont;
	x509v3ext ext;
        if (!crlDist->text().isEmpty()) {
		ext.create(NID_crl_distribution_points, crlDist->text());
	}
	return ext;
}

extList NewX509::getNetscapeExt()
{
	QString certTypeList[] = {
		"client", "server",  "email", "objsign",
		"sslCA",  "emailCA", "objCA" };

					
	QStringList cont;
	x509v3ext ext;
	extList el;
	QListBoxItem *item;
        for (int i=0; (item = nsCertType->item(i)); i++) {
                if (item->selected()){
                        cont <<  certTypeList[i];
                }
        }
	el << ext.create(NID_netscape_cert_type, cont.join(", "))
	<< ext.create(NID_netscape_base_url, nsBaseUrl->text())
	<< ext.create(NID_netscape_revocation_url, nsRevocationUrl->text())
	<< ext.create(NID_netscape_ca_revocation_url, nsCARevocationUrl->text())
	<< ext.create(NID_netscape_renewal_url, nsRenewalUrl->text())
	<< ext.create(NID_netscape_ca_policy_url, nsCaPolicyUrl->text())
	<< ext.create(NID_netscape_ssl_server_name, nsSslServerName->text())
	<< ext.create(NID_netscape_comment, nsComment->text());
	return el;
}

void NewX509::initCtx(pki_x509 *subj)
{
	pki_x509 *iss = getSelectedSigner();
	X509 *s = NULL, *s1 = NULL;
	if (iss) s = iss->getCert();
	if (subj) s1 = subj->getCert();
	
	X509V3_set_ctx(&ext_ctx, s , s1, NULL, NULL, 0);
}	

void NewX509::setExt(const x509v3ext &ext)
{
	switch (ext.nid()) {
		case NID_basic_constraints:
			bcCritical->setChecked(ext.getCritical());
	}
}