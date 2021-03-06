/* email.cpp */

/* Synchronet email function - for sending private e-mail */

/* $Id: email.cpp,v 1.41 2006/08/23 22:34:32 rswindell Exp $ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright 2006 Rob Swindell - http://www.synchro.net/copyright.html		*
 *																			*
 * This program is free software; you can redistribute it and/or			*
 * modify it under the terms of the GNU General Public License				*
 * as published by the Free Software Foundation; either version 2			*
 * of the License, or (at your option) any later version.					*
 * See the GNU General Public License for more details: gpl.txt or			*
 * http://www.fsf.org/copyleft/gpl.html										*
 *																			*
 * Anonymous FTP access to the most recent released source is available at	*
 * ftp://vert.synchro.net, ftp://cvs.synchro.net and ftp://ftp.synchro.net	*
 *																			*
 * Anonymous CVS access to the development source and modification history	*
 * is available at cvs.synchro.net:/cvsroot/sbbs, example:					*
 * cvs -d :pserver:anonymous@cvs.synchro.net:/cvsroot/sbbs login			*
 *     (just hit return, no password is necessary)							*
 * cvs -d :pserver:anonymous@cvs.synchro.net:/cvsroot/sbbs checkout src		*
 *																			*
 * For Synchronet coding style and modification guidelines, see				*
 * http://www.synchro.net/source.html										*
 *																			*
 * You are encouraged to submit any modifications (preferably in Unix diff	*
 * format) via e-mail to mods@synchro.net									*
 *																			*
 * Note: If this box doesn't appear square, then you need to fix your tabs.	*
 ****************************************************************************/

#include "sbbs.h"
#include "cmdshell.h"

/****************************************************************************/
/* Mails a message to usernumber. 'top' is a buffer to place at beginning   */
/* of message.                                                              */
/* Called from functions main_sec, newuser, readmail and scanposts			*/
/****************************************************************************/
bool sbbs_t::email(int usernumber, char *top, char *subj, long mode)
{
	char	str[256],str2[256],msgpath[256],title[LEN_TITLE+1],ch
			,buf[SDT_BLOCK_LEN];
	char 	tmp[512];
	ushort	xlat=XLAT_NONE,msgattr=0;
	ushort	nettype;
	int 	i,j,x,file;
	long	l;
	ulong	length,offset,crc=0xffffffffUL;
	FILE	*instream;
	node_t	node;
	smbmsg_t msg;

	SAFECOPY(title,subj);

	if(useron.etoday>=cfg.level_emailperday[useron.level] && !SYSOP) {
		bputs(text[TooManyEmailsToday]);
		return(false); }

	if(usernumber==1 && useron.rest&FLAG('S')
		&& (cfg.node_valuser!=1 || useron.fbacks || useron.emails)) { /* ! val fback */
		bprintf(text[R_Feedback],cfg.sys_op);
		return(false); }
	if(usernumber!=1 && useron.rest&FLAG('E')
		&& (cfg.node_valuser!=usernumber || useron.fbacks || useron.emails)) {
		bputs(text[R_Email]);
		return(false); }
	if(!usernumber) {
		bputs(text[UnknownUser]);
		return(false); }
	getuserrec(&cfg,usernumber,U_MISC,8,str);
	l=ahtoul(str);
	if(l&(DELETED|INACTIVE)) {              /* Deleted or Inactive User */
		bputs(text[UnknownUser]);
		return(false); }
	if(l&NETMAIL && cfg.sys_misc&SM_FWDTONET) {
		getuserrec(&cfg,usernumber,U_NETMAIL,LEN_NETMAIL,str);
		bprintf(text[UserNetMail],str);
		if(yesno(text[ForwardMailQ])) /* Forward to netmail address */
			return(netmail(str,subj,mode));
	}
	bprintf(text[Emailing],username(&cfg,usernumber,tmp),usernumber);
	action=NODE_SMAL;
	nodesync();

	sprintf(str,"%sfeedback.*", cfg.exec_dir);
	if(usernumber==1 && useron.fbacks && fexist(str)) {
		exec_bin("feedback",&main_csi);
		if(main_csi.logic!=LOGIC_TRUE)
			return(false); }

	if(cfg.sys_misc&SM_ANON_EM && useron.exempt&FLAG('A')
		&& !noyes(text[AnonymousQ]))
		msgattr|=MSG_ANONYMOUS;

	if(cfg.sys_misc&SM_DELREADM)
		msgattr|=MSG_KILLREAD;

	msg_tmp_fname(useron.xedit, msgpath, sizeof(msgpath));
	username(&cfg,usernumber,str2);
	if(!writemsg(msgpath,top,title,mode,INVALID_SUB,str2)) {
		bputs(text[Aborted]);
		return(false); 
	}

	if(mode&WM_FILE && !SYSOP && !(cfg.sys_misc&SM_FILE_EM))
		mode&=~WM_FILE;


	if(mode&WM_FILE) {
		sprintf(str2,"%sfile/%04u.in", cfg.data_dir,usernumber);
		MKDIR(str2);
		sprintf(str2,"%sfile/%04u.in/%s", cfg.data_dir,usernumber,title);
		if(fexistcase(str2)) {
			bputs(text[FileAlreadyThere]);
			remove(msgpath);
			return(false); }
#if 0	/* no such thing as local logon */
		if(online==ON_LOCAL) {		/* Local upload */
			bputs(text[EnterPath]);
			if(!getstr(str,60,K_LINE|K_UPPER)) {
				bputs(text[Aborted]);
				remove(msgpath);
				return(false); }
			backslash(str);
			strcat(str,title);
			mv(str,str2,1); }
		else 
#endif
		{ /* Remote */
			xfer_prot_menu(XFER_UPLOAD);
			mnemonics(text[ProtocolOrQuit]);
			strcpy(str,"Q");
			for(x=0;x<cfg.total_prots;x++)
				if(cfg.prot[x]->ulcmd[0] && chk_ar(cfg.prot[x]->ar,&useron)) {
					sprintf(tmp,"%c",cfg.prot[x]->mnemonic);
					strcat(str,tmp); }
			ch=(char)getkeys(str,0);
			if(ch=='Q' || sys_status&SS_ABORT) {
				bputs(text[Aborted]);
				remove(msgpath);
				return(false); }
			for(x=0;x<cfg.total_prots;x++)
				if(cfg.prot[x]->ulcmd[0] && cfg.prot[x]->mnemonic==ch
					&& chk_ar(cfg.prot[x]->ar,&useron))
					break;
			if(x<cfg.total_prots)	/* This should be always */
				protocol(cfg.prot[x],XFER_UPLOAD,str2,nulstr,true); 
		}
		sprintf(tmp,"%s%s",cfg.temp_dir,title);
		if(!fexistcase(str2) && fexistcase(tmp))
			mv(tmp,str2,0);
		l=flength(str2);
		if(l>0)
			bprintf(text[FileNBytesReceived],title,ultoac(l,tmp));
		else {
			bprintf(text[FileNotReceived],title);
			remove(msgpath);
			return(false); } }

	bputs(text[WritingIndx]);

	if((i=smb_stack(&smb,SMB_STACK_PUSH))!=0) {
		errormsg(WHERE,ERR_OPEN,"MAIL",i);
		return(false); }
	sprintf(smb.file,"%smail", cfg.data_dir);
	smb.retry_time=cfg.smb_retry_time;
	smb.subnum=INVALID_SUB;
	if((i=smb_open(&smb))!=0) {
		smb_stack(&smb,SMB_STACK_POP);
		errormsg(WHERE,ERR_OPEN,smb.file,i,smb.last_error);
		return(false); }

	if(smb_fgetlength(smb.shd_fp)<1) {	 /* Create it if it doesn't exist */
		smb.status.max_crcs=cfg.mail_maxcrcs;
		smb.status.max_age=cfg.mail_maxage;
		smb.status.max_msgs=0;
		smb.status.attr=SMB_EMAIL;
		if((i=smb_create(&smb))!=0) {
			smb_close(&smb);
			smb_stack(&smb,SMB_STACK_POP);
			errormsg(WHERE,ERR_CREATE,smb.file,i,smb.last_error);
			return(false); } }

	if((i=smb_locksmbhdr(&smb))!=0) {
		smb_close(&smb);
		smb_stack(&smb,SMB_STACK_POP);
		errormsg(WHERE,ERR_LOCK,smb.file,i,smb.last_error);
		return(false); }

	length=flength(msgpath)+2;	 /* +2 for translation string */

	if(length&0xfff00000UL) {
		smb_unlocksmbhdr(&smb);
		smb_close(&smb);
		smb_stack(&smb,SMB_STACK_POP);
		errormsg(WHERE,ERR_LEN,msgpath,length);
		return(false); }

	if((i=smb_open_da(&smb))!=0) {
		smb_unlocksmbhdr(&smb);
		smb_close(&smb);
		smb_stack(&smb,SMB_STACK_POP);
		errormsg(WHERE,ERR_OPEN,smb.file,i,smb.last_error);
		return(false); }
	if(cfg.sys_misc&SM_FASTMAIL)
		offset=smb_fallocdat(&smb,length,1);
	else
		offset=smb_allocdat(&smb,length,1);
	smb_close_da(&smb);

	if((file=open(msgpath,O_RDONLY|O_BINARY))==-1
		|| (instream=fdopen(file,"rb"))==NULL) {
		smb_freemsgdat(&smb,offset,length,1);
		smb_unlocksmbhdr(&smb);
		smb_close(&smb);
		smb_stack(&smb,SMB_STACK_POP);
		errormsg(WHERE,ERR_OPEN,msgpath,O_RDONLY|O_BINARY);
		return(false); }

	setvbuf(instream,NULL,_IOFBF,2*1024);
	smb_fseek(smb.sdt_fp,offset,SEEK_SET);
	xlat=XLAT_NONE;
	smb_fwrite(&smb,&xlat,2,smb.sdt_fp);
	x=SDT_BLOCK_LEN-2;				/* Don't read/write more than 255 */
	while(!feof(instream)) {
		memset(buf,0,x);
		j=fread(buf,1,x,instream);
		if(j<1)
			break;
		if(j>1 && (j!=x || feof(instream)) && buf[j-1]==LF && buf[j-2]==CR)
			buf[j-1]=buf[j-2]=0;
		if(cfg.mail_maxcrcs) {
			for(i=0;i<j;i++)
				crc=ucrc32(buf[i],crc); }
		smb_fwrite(&smb,buf,j,smb.sdt_fp);
		x=SDT_BLOCK_LEN; }
	smb_fflush(smb.sdt_fp);
	fclose(instream);
	crc=~crc;

	memset(&msg,0,sizeof(smbmsg_t));
	msg.hdr.version=smb_ver();
	msg.hdr.attr=msgattr;
	if(mode&WM_FILE)
		msg.hdr.auxattr|=MSG_FILEATTACH;
	msg.hdr.when_written.time=msg.hdr.when_imported.time=time(NULL);
	msg.hdr.when_written.zone=msg.hdr.when_imported.zone=sys_timezone(&cfg);

	if(cfg.mail_maxcrcs) {
		i=smb_addcrc(&smb,crc);
		if(i) {
			smb_freemsgdat(&smb,offset,length,1);
			smb_unlocksmbhdr(&smb);
			smb_close(&smb);
			smb_stack(&smb,SMB_STACK_POP);
			attr(cfg.color[clr_err]);
			bputs("Duplicate message!\r\n");
			return(false); 
		} 
	}

	msg.hdr.offset=offset;

	username(&cfg,usernumber,str);
	smb_hfield_str(&msg,RECIPIENT,str);

	sprintf(str,"%u",usernumber);
	smb_hfield_str(&msg,RECIPIENTEXT,str);

	strcpy(str,useron.alias);
	smb_hfield_str(&msg,SENDER,str);

	sprintf(str,"%u",useron.number);
	smb_hfield_str(&msg,SENDEREXT,str);

	if(useron.misc&NETMAIL) {
		nettype=smb_netaddr_type(useron.netmail);
		if(nettype!=NET_NONE && nettype!=NET_UNKNOWN) {
			smb_hfield(&msg,REPLYTONETTYPE,sizeof(nettype),&nettype);
			smb_hfield_str(&msg,REPLYTONETADDR,useron.netmail);
		}
	}

	/* Security logging */
	msg_client_hfields(&msg,&client);

	smb_hfield_str(&msg,SUBJECT,title);

	smb_dfield(&msg,TEXT_BODY,length);

	i=smb_addmsghdr(&smb,&msg,SMB_SELFPACK); // calls smb_unlocksmbhdr() 
	smb_close(&smb);
	smb_stack(&smb,SMB_STACK_POP);

	smb_freemsgmem(&msg);
	if(i!=SMB_SUCCESS) {
		smb_freemsgdat(&smb,offset,length,1);
		errormsg(WHERE,ERR_WRITE,smb.file,i,smb.last_error);
		return(false); 
	}

	if(usernumber==1)
		logon_fbacks++;
	else
		logon_emails++;
	user_sent_email(&cfg, &useron, 1, usernumber==1);
	bprintf(text[Emailed],username(&cfg,usernumber,tmp),usernumber);
	sprintf(str,"%s sent e-mail to %s #%d"
		,useron.alias,username(&cfg,usernumber,tmp),usernumber);
	logline("E+",str);
	if(mode&WM_FILE && online==ON_REMOTE)
		autohangup();
	if(msgattr&MSG_ANONYMOUS)				/* Don't tell user if anonymous */
		return(true);
	for(i=1;i<=cfg.sys_nodes;i++) { /* Tell user, if online */
		getnodedat(i,&node,0);
		if(node.useron==usernumber && !(node.misc&NODE_POFF)
			&& (node.status==NODE_INUSE || node.status==NODE_QUIET)) {
			sprintf(str,text[EmailNodeMsg],cfg.node_num,useron.alias);
			putnmsg(&cfg,i,str);
			break; } }
	if(i>cfg.sys_nodes) {	/* User wasn't online, so leave short msg */
		sprintf(str,text[UserSentYouMail],useron.alias);
		putsmsg(&cfg,usernumber,str); }
	return(true);
}
