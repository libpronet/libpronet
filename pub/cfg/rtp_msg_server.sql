CREATE TABLE [tbl_msg01_user] (
  [_cid_] SMALLINT NOT NULL,
  [_uid_] BIGINT NOT NULL,
  [_maxiids_] INT,
  [_isc2s_] BOOLEAN,
  [_passwd_] VARCHAR(64),
  [_bindedip_] VARCHAR(64),
  [_description_] TEXT,
  CHECK(_cid_>0 AND _cid_<=255 AND _uid_>=0),
  CONSTRAINT [sqlite_autoindex_tbl_msg01_user_1] PRIMARY KEY ([_cid_], [_uid_]));


CREATE TABLE [tbl_msg02_kickout] (
  [_cid_] SMALLINT NOT NULL,
  [_uid_] BIGINT NOT NULL,
  [_iid_] SMALLINT NOT NULL);


CREATE TABLE [tbl_msg03_online] (
  [_cid_] SMALLINT NOT NULL,
  [_uid_] BIGINT NOT NULL,
  [_iid_] SMALLINT NOT NULL,
  [_fromip_] VARCHAR(64),
  [_fromc2s_] VARCHAR(64),
  [_sslsuite_] VARCHAR(64),
  [_logontime_] DATETIME,
  CHECK(_cid_>0 AND _cid_<=255 AND _uid_>0 AND _iid_>0 AND _iid_<=65535),
  CONSTRAINT [sqlite_autoindex_tbl_msg03_online_1] PRIMARY KEY ([_cid_], [_uid_], [_iid_]));


INSERT INTO tbl_msg01_user (_cid_, _uid_, _isc2s_, _passwd_, _description_) VALUES (1, 10000001, 1, 'test', 'c2s-10000001');
INSERT INTO tbl_msg01_user (_cid_, _uid_, _isc2s_, _passwd_, _description_) VALUES (1, 0       , 0, 'test', '1-0'         );
INSERT INTO tbl_msg01_user (_cid_, _uid_, _isc2s_, _passwd_, _description_) VALUES (2, 0       , 0, 'test', '2-0'         );
