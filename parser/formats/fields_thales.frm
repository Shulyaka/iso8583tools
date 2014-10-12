#Yes, the parser is capable of parse and build Racal messages!
#Number		Format		Description

key		F33SF		Encryption key format template
key.0		F1ASC=U		Key scheme (Variant)
key.1		F32ASC		Double length DES key
key		F5SF		Encryption key
key.0		F1ASC=U		Key scheme (Double length DES Variant)
key.1		F1ASC=K		User storage indicator
key.2		F3ASC		key index
key		F17SF		Encryption key
key.0		F1ASC=Z		Key scheme (Variant)
key.1		F16ASC		Single length DES key
key		F5SF		Encryption key
key.0		F1ASC=Z		Key scheme (Single length DES Variant)
key.1		F1ASC=K		User storage indicator
key.2		F3ASC		key index
key		F49SF		Encryption key
key.0		F1ASC=T		Key scheme (Variant)
key.1		F48ASC		Triple length DES key
key		F5SF		Encryption key
key.0		F1ASC=T		Key scheme (Triple length DES Variant)
key.1		F1ASC=K		User storage indicator
key.2		F3ASC		key index
key		F33SF		Encryption key template
key.0		F1ASC=X		Key scheme
key.1		F32ASC		Double length DES key
key		F5SF		Encryption key
key.0		F1ASC=X		Key scheme (Double length DES)
key.1		F1ASC=K		User storage indicator
key.2		F3ASC		key index
key		F49SF		Encryption key
key.0		F1ASC=Y		Key scheme
key.1		F48ASC		Triple length DES key
key		F5SF		Encryption key
key.0		F1ASC=Y		Key scheme (Triple length DES)
key.1		F1ASC=K		User storage indicator
key.2		F3ASC		key index
key		F4SF		Encryption key (Single length DES)
key.1		F1ASC=K		User storage indicator
key.2		F3ASC		key index
key		F16SF		Encryption key (no key scheme)
key.1		F16ASC		Single length DES key


message 	BB1024SF	Thales HSM message
0		F4ASC		Message Header

# Symmetric Key Management Commands

#1		U130SF		Generate a Key
#1.0		F2ASC=A0	Command Code
#1.1		U5SF		Mode
#1.1.0		F1ASC=0		Mode (Generate key)
#1.1.1		F3ASC		Key Type
#1.1.2		F1ASC		Key Scheme (LMK)
#1.1		U59SF		Mode
#1.1.0		F1ASC=1		Mode (Generate key and encrypt under ZMK (or TMK))
#1.1.1		F3ASC		Key Type
#1.1.2		F1ASC		Key Scheme (LMK)
#1.1.7		F2SF		ZMK/TMK Flag
#1.1.7.0		F1ASC=;		Delimiter
#1.1.7.1		F1ASC		ZMK/TMK Flag
#1.1.7		F0		ZMK/TMK Flag
#1.1.8		Rkey		ZMK (or TMK)
#1.1.9		F1ASC		Key Scheme (ZMK or TMK)
#1.1.10		U2ASC		Atalla Variant
#1.1		U71SF		Mode
#1.1.0		F1ASC=A		Mode (Derive key)
#1.1.1		F3ASC		Key Type
#1.1.2		F1ASC		Key Scheme (LMK)
#1.1.3		F1ASC=O		Derive Key Mode
#1.1.4		F1ASC		DUKPT Master Key Type
#1.1.5		Rkey		DUKPT Master Key
#1.1.6		F15ASC		KSN
#1.1		U125SF		Mode
#1.1.0		F1ASC=B		Mode (Derive key and encrypt under ZMK (or TMK)
#1.1.1		F3ASC		Key Type
#1.1.2		F1ASC		Key Scheme (LMK)
#1.1.3		F1ASC=O		Derive Key Mode
#1.1.4		F1ASC		DUKPT Master Key Type
#1.1.5		Rkey		DUKPT Master Key
#1.1.6		F15ASC		KSN
#1.1.7		F2SF		ZMK/TMK Flag
#1.1.7.0		F1ASC=;		Delimiter
#1.1.7.1		F1ASC		ZMK/TMK Flag
#1.1.7		F0		ZMK/TMK Flag
#1.1.8		Rkey		ZMK (or TMK)
#1.1.9		F1ASC		Key Scheme (ZMK or TMK)
#1.1.10		U2ASC		Atalla Variant
#1.2		F3SF		LMK Identifier
#1.2.0		F1ASC=%		Delimiter
#1.2.1		F2ASC		LMK Identifier
#1.2		F0		LMK Identifier
#1.3		U519SF		Export options
#1.3.0		F1ASC=&		Delimiter
#1.3.1		F2ASC		Key Usage
#1.3.2		F1ASC		Mode of Use
#1.3.3		F2ASC		Key Version Number
#1.3.4		F1ASC		Exportability
#1.3.5		F2ASC		Number of Optional Blocks
#1.3.6		U255SF		Optional block 1
#1.3.6.0		F2ASC		Optional Block Identifier
#1.3.6.1		AA253		Optional Block Data
#1.3.7		R1.3.6		Optional block 2
#1.3.8		R1.3.6		Optional block 3

1		U649SF		Generate a Key
1.0		F2ASC=A0	Command Code
1.1		F1ASC		Mode
1.2		F3ASC		Key Type
1.3		F1ASC		Key Scheme (LMK)
1.4		U66SF		DUKPT
1.4.0		F1ASC=O		Derive Key Mode
1.4.1		F1ASC		DUKPT Master Key Type
1.4.2		Rkey		DUKPT Master Key
1.4.3		F15ASC		KSN
1.4		F0		DUKPT
1.5		U2SF		ZMK/TMK Flag
1.5.0		F1ASC=;		Delimiter
1.5.1		F1ASC		ZMK/TMK Flag
1.5		F0		ZMK/TMK Flag
1.6		U52SF		ZMK (or TMK)
1.6.0		Rkey		ZMK (or TMK)
1.6.1		F1ASC		Key Scheme (ZMK or TMK)
1.6.2		U2ASC		Atalla Variant
1.6		F0		ZMK (or TMK)
1.7		F3SF		LMK Identifier
1.7.0		F1ASC=%		Delimiter
1.7.1		F2ASC		LMK Identifier
1.7		F0		LMK Identifier
1.8		U519SF		Export options
1.8.0		F1ASC=&		Delimiter
1.8.1		F2ASC		Key Usage
1.8.2		F1ASC		Mode of Use
1.8.3		F2ASC		Key Version Number
1.8.4		F1ASC		Exportability
1.8.5		F2ASC		Number of Optional Blocks
1.8.6		U255SF		Optional block 1
1.8.6.0		F2ASC		Optional Block Identifier
1.8.6.1		AA253		Optional Block Data
1.8.7		R1.8.6		Optional block 2
1.8.8		R1.8.6		Optional block 3
1		U SF		Generate a Key (Response)
1.0		F2ASC=A1	Response code
1.1		F2ASC		Error code
1.2		Rkey		Key (under LMK)
1.3		Rkey		Key (under ZMK or TMK)
1.3		F0		Key (under ZMK or TMK)
1.4		F6ASC		Key Check Value

1		U2570SF		Generate and Print a Component
1.0		F2ASC=A2	Command Code
1.1		F3ASC		Key Type
1.2		F1ASC=1		Component Check Flag (Do not return component check value)
1.2		F1ASC=2		Component Check Flag (Return component check value)
1.2		F0		Component Check Flag
1.3		F1ASC		Key Scheme (LMK)
1.4		U2560SF		Print Fields
1.4.0		U255SF		Print Field 0
1.4.0.1		U255ASC		Print Field data
1.4.1		U256SF		Print Field 1
1.4.1.0		F1ASC=;		Delimiter
1.4.1.1		U255ASC		Print Field data
1.4.2		R1.4.1		Print Field 2
1.4.3		R1.4.1		Print Field 3
1.4.4		R1.4.1		Print Field 4
1.4.5		R1.4.1		Print Field 5
1.4.6		R1.4.1		Print Field 6
1.4.7		R1.4.1		Print Field 7
1.4.8		R1.4.1		Print Field 8
1.4.9		R1.4.1		Print Field 9
1.5		F1ASC=~		Delimiter
1.6		F1ASC=%		Delimiter
1.7		F2ASC		LMK Identifier
1		U56SF		Generate and Print a Component (Response)
1.0		F2ASC=A3	Response Code (before printing)
1.1		F2ASC		Error Code
1.2		Rkey		Component
1.3		F3ASC		Component Check Value
1		U4SF		Generate and Print a Component (Response)
1.0		F2ASC=AZ	Response Code (after printing)
1.1		F2ASC		Error Code

1		U2570SF		Generate and Print a Key as Split Components
1.0		F2ASC=NE	Command Code
1.1		F3ASC		Key Type
1.2		F1ASC		Key Scheme (LMK)
1.3		U2560SF		Print Fields
1.3.0		U255SF		Print Field 0
1.3.0.1		U255ASC		Print Field data
1.3.1		U256SF		Print Field 1
1.3.1.0		F1ASC=;		Delimiter
1.3.1.1		U255ASC		Print Field data
1.3.2		R1.3.1		Print Field 2
1.3.3		R1.3.1		Print Field 3
1.3.4		R1.3.1		Print Field 4
1.3.5		R1.3.1		Print Field 5
1.3.6		R1.3.1		Print Field 6
1.3.7		R1.3.1		Print Field 7
1.3.8		R1.3.1		Print Field 8
1.3.9		R1.3.1		Print Field 9
1.4		F1ASC=~		Delimiter
1.5		F1ASC=%		Delimiter
1.5		F2ASC		LMK Identifier
1		U56SF		Generate and Print a Key as Split Components (Response)
1.0		F2ASC=NF	Response Code (before printing)
1.1		F2ASC		Error Code
1.2		Rkey		Key
1.3		F3ASC		Key Check Value
1		U4SF		Generate and Print a Key as Split Components (Response)
1.0		F2ASC=NZ	Response Code (after printing)
1.1		F2ASC		Error Code

1		U451SF		Form a Key from Encrypted Components
1.0		F2ASC=A4	Command Code
1.1		F1ASC		Number of components
1.2		F3ASC		Key Type
1.3		F1ASC		Key Scheme (LMK)
1.4		U441SF		Key components
1.4.1		Rkey		Key Component 1
1.4.2		Rkey		Key Component 2
1.4.3		Rkey		Key Component 3
1.4.4		Rkey		Key Component 4
1.4.5		Rkey		Key Component 5
1.4.6		Rkey		Key Component 6
1.4.7		Rkey		Key Component 7
1.4.8		Rkey		Key Component 8
1.4.9		Rkey		Key Component 9
1.5		F1ASC=%		Delimiter
1.6		F2ASC		LMK Identifier
1		U59SF		Form a Key from Encrypted Components (Response)
1.0		F2ASC=A5	Response Code
1.1		F2ASC		Error Code
1.2		Rkey		Key (under LMK)
1.3		F6ASC		Key Check Value

1		U109SF		Import a Key
1.0		F2ASC=A6	Command Code
1.1		F3ASC		Key Type
1.2		Rkey		ZMK
1.3		Rkey		Key (ZMK)
1.4		F1ASC		Key Scheme (LMK)
1.5		U2ASC		Atalla Variant
1.5		F0		Atalla Variant
1.6		F1ASC=%		Delimiter
1.7		F2ASC		LMK Identifier
1		U59SF		Import a Key (Response)
1.0		F2ASC=A7	Response Code
1.1		F2ASC		Error Code
1.2		Rkey		Key (under LMK)
1.3		F6ASC		Key Check Value

1		U113SF		Export a Key
1.0		F2ASC=A8	Command Code
1.1		F3ASC		Key Type
1.2		U2SF		ZMK/TMK Flag
1.2.0		F1ASC=;		Delimiter
1.2.1		F1ASC		ZMK/TMK Flag
1.2		F0		ZMK/TMK Flag
1.3		Rkey		ZMK (or TMK)
1.4		Rkey		Rkey
1.5		F1ASC		Key Scheme (ZMK or TMK)
1.6		U2ASC		Atalla Variant
1.6		F0		Atalla Variant
1.7		F1ASC=%		Delimiter
1.8		F2ASC		LMK Identifier
1		U59SF		Export a Key (Response)
1.0		F2ASC=A8	Response Code
1.1		F2ASC		Error Code
1.2		Rkey		Key (under ZMK or TMK)
1.3		F6ASC		Key Check Value

1		U58SF		Translate Key Scheme
1.0		F2ASC=B0	Command Code
1.1		F3ASC		Key Type
1.2		Rkey		Key
1.3		F1ASC		Key Scheme (LMK)
1.5		F1ASC=%		Delimiter
1.6		F2ASC		LMK Identifier
1		U53SF		Translate Key Scheme (Response)
1.0		F2ASC=B1	Response Code
1.1		F2ASC		Error Code
1.2		Rkey		Key

1		U103SF		Translate ZMK from ZMK to LMK encryption
1.0		F2ASC=BY	Command Code
1.1		Rkey		ZMKi
1.2		Rkey		ZMK
1.3		U2ASC		Atalla Variant
1.3		F0		Atalla Variant
1.4		F4SF		Options
1.4.0		F1ASC=;		Delimiter
1.4.1		F1ASC=0		Reserved
1.4.2		F1ASC		Key Scheme (LMK)
1.4.3		F1ASC		Key Check Value Type
1.5		F1ASC=%		Delimiter
1.6		F2ASC		LMK Identifier
1		U59SF		Translate ZMK from ZMK to LMK encryption (Response)
1.0		F2ASC=BZ	Response Code
1.1		F2ASC		Error Code
1.2		Rkey		ZMK
1.3		F6ASC		Key Check Value

1		U58SF		Generate a TMK, TPK or PVK
1.0		F2ASC=HC	Command Code
1.1		Rkey		Current TMK, TPK or PVK
1.2		F4SF		Options
1.2.0		F1ASC=;		Delimiter
1.2.1		F1ASC		Key Scheme (TMK)
1.2.2		F1ASC		Key Scheme (LMK)
1.2.3		F1ASC=0		Reserved
1.3		F1ASC=%		Delimiter
1.4		F2ASC		LMK Identifier
1		U102SF		Generate a TMK, TPK or PVK (Response)
1.0		F2ASC=HD	Response Code
1.1		F2ASC		Error Code
1.2		Rkey		New key under the current key
1.3		Rkey		New key under LMK

1		U58SF		Generate a TAK
1.0		F2ASC=HA	Command Code
1.1		Rkey		TMK
1.2		F4SF		Options
1.2.0		F1ASC=;		Delimiter
1.2.1		F1ASC		Key Scheme (TMK)
1.2.2		F1ASC		Key Scheme (LMK)
1.2.3		F1ASC=0		Reserved
1.3		F1ASC=%		Delimiter
1.4		F2ASC		LMK Identifier
1		U102SF		Generate a TMK, TPK or PVK (Response)
1.0		F2ASC=HB	Response Code
1.1		F2ASC		Error Code
1.2		Rkey		TAK under TMK
1.3		Rkey		TAK under LMK


1		U107SF		Translate a TMK, TPK or PVK from LMK to Another TMK, TPK or PVK
1.0		F2ASC=AE	Command Code
1.1		Rkey		Current TMK, TPK or PVK
1.2		Rkey		Stored TMK, TPK or PVK
1.3		F4SF		Options
1.3.0		F1ASC=;		Delimiter
1.3.1		F1ASC		Key Scheme (TMK)
1.3.2		F1ASC=0		Reserved
1.3.3		F1ASC=0		Reserved
1.4		F1ASC=%		Delimiter
1.5		F2ASC		LMK Identifier
1		U53SF		Translate a TMK, TPK or PVK from LMK to Another TMK, TPK or PVK (Response)
1.0		F2ASC=AF	Response Code
1.1		F2ASC		Error Code
1.2		Rkey		Stored key under the current key

1		U107SF		Translate a TAK from LMK to TMK Encryption
1.0		F2ASC=AG	Command Code
1.1		Rkey		TMK
1.2		Rkey		TAK
1.3		F4SF		Options
1.3.0		F1ASC=;		Delimiter
1.3.1		F1ASC		Key Scheme (TMK)
1.3.2		F1ASC=0		Reserved
1.3.3		F1ASC=0		Reserved
1.4		F1ASC=%		Delimiter
1.5		F2ASC		LMK Identifier
1		U53SF		Translate a TAK from LMK to TMK Encryption (Response)
1.0		F2ASC=AH	Response Code
1.1		F2ASC		Error Code
1.2		Rkey		Translated TAK; encrypted under the TMK

# PIN and Offset Generation Commands

pin		F33SF		PIN template
pin.0		F1ASC=J		Identifier
pin.1		F32ASC		PIN (AES)
pin		U16SF		PIN
pin.1		U16ASC		PIN

dectab		F33SF		Decimalization Table template
dectab.0	F1ASC=L		Identifier (AES Keyblock LMK)
dectab.1	F32ASC		Decimalization Table Data
dectab		F4SF		Decimalization Table (User Storage Area)
dectab.0	F1ASC=K		Identifier
dectab.1	F3ASC		Position
dectab		F16SF		Decimalization Table (Plaintext or Encrypted)
dectab.1	F15ASC		Decimalization Table Data

pval		F17SF		PIN Validation Data template
pval.0		F1ASC=P		Identifier
pval.1		F16ASC		PIN Validation Data
pval		F12SF		PIN Validation Data
pval.1		F12ASC		PIN Validation Data

excl		U1193SF		Excluded PIN Table template
excl.0		F1ASC=*		Delimiter
excl.1		F2ASC		Excluded PIN Count
excl.2		F2ASC		Excluded PIN Length
excl.3		U1188ASC	Excluded PIN Table
excl		F0		Excluded PIN Table

1		U1327SF		Derive a PIN Using the IBM Method
1.0		F2ASC=EE	Command Code
1.1		Rkey		PVK
1.2		F16ASC		Offset
1.3		F2ASC		Check Length
1.4		F12ASC		Account Number
1.5		Rdectab		Decimalization Table
1.6		Rpval		PIN Validation Data
1.7		Rexcl		Excluded PIN Table
1.8		F1ASC=%		Delimiter
1.9		F2ASC		LMK Identifier
1		U36SF		Derive a PIN Using the IBM Method (Response)
1.0		F2ASC=EF	Response Code
1.1		F2ASC		Error Code
1.2		Rpin		PIN

1		U43SF		Derive a PIN Using the Diebold Method
1.0		F2ASC=GA	Command Code
1.1		F4SF		PVK
1.1.0		F1ASC=K		Index Flag
1.1.1		F3ASC		Table Pointer
1.2		F2ASC		Algorithm Number
1.3		F4ASC		Offset
1.4		F12ASC		Account Number
1.5		F16ASC		Validation Data
1.6		F1ASC=%		Delimiter
1.7		F2ASC		LMK Identifier
1		U36SF		Derive a PIN Using the Diebold Method (Response)
1.0		F2ASC=GB	Response Code
1.1		F2ASC		Error Code
1.2		Rpin		PIN

1		U1212SF		Generate a Random PIN
1.0		F2ASC=JA	Command Code
1.1		F12ASC		Account Number
1.2		F2ASC		PIN Length
1.2		F0		PIN Length = 4
1.3		Rexcl		Excluded PIN Table
1.4		F1ASC=%		Delimiter
1.5		F2ASC		LMK Identifier
1		U36SF		Generate a Random PIN (Response)
1.0		F2ASC=JB	Response Code
1.1		F2ASC		Error Code
1.2		Rpin		PIN

1		U1244SF		Generate an IBM PIN Offset (of an LMK encrypted PIN)
1.0		F2ASC=DE	Command Code
1.1		Rkey		PVK
1.2		Rpin		PIN
1.3		F2ASC		Check Length
1.4		F12ASC		Account Number
1.5		Rdectab		Decimalization Table
1.6		Rpval		PIN Validation Data
1.7		Rexcl		Excluded PIN Table
1.8		F1ASC=%		Delimiter
1.9		F2ASC		LMK Identifier
1		U36SF		Generate an IBM PIN Offset (Response)
1.0		F2ASC=DF	Response Code
1.1		F2ASC		Error Code
1.2		F12ASC		Offset

1		U1381SF		Generate an IBM PIN Offset (of a customer selected PIN)
1.0		F2ASC=BK	Command Code
1.1		F3ASC		PIN Block Key Type
1.2		Rkey		PIN Block Key
1.3		Rkey		PVK
1.4		F16ASC		PIN Block
1.5		F2ASC		PIN Block Format Code
1.6		F2ASC		Check Length
1.7		F12ASC		Account Number
1.8		Rdectab		Decimalization Table
1.9		Rpval		PIN Validation Data
1.10		Rexcl		Excluded PIN Table
1.11		F1ASC=%		Delimiter
1.12		F2ASC		LMK Identifier
1		U36SF		Generate an IBM PIN Offset (Response)
1.0		F2ASC=BL	Response Code
1.1		F2ASC		Error Code
1.2		F12ASC		Offset

1		U1265SF		Generate a Diebold PIN Offset
1.0		F2ASC=CE	Command Code
1.1		F4SF		PVK
1.1.0		F1ASC=K		Index Flag
1.1.1		F3ASC		Table Pointer
1.2		F2ASC		Algorithm Number
1.3		Rpin		PIN
1.4		F12ASC		Account Number
1.5		F16ASC		Validation Data
1.6		Rexcl		Excluded PIN Table
1.7		F1ASC=%		Delimiter
1.9		F2ASC		LMK Identifier
1		U8SF		Generate a Diebold PIN Offset (Response)
1.0		F2ASC=CF	Response Code
1.1		F2ASC		Error Code
1.2		F4ASC		Offset

1		U1293SF		Generate a VISA PIN Verification Value (of an LMK encrypted PIN)
1.0		F2ASC=DG	Command Code
1.1		Rkey		PVK Pair
1.2		Rpin		PIN
1.3		F12ASC		Account Number
1.4		F1ASC		PVKI
1.5		Rexcl		Excluded PIN Table
1.6		F1ASC=%		Delimiter
1.7		F2ASC		LMK Identifier
1		U8SF		Generate a VISA PIN Verification Value (Response)
1.0		F2ASC=DH	Response Code
1.1		F2ASC		Error Code
1.2		F4ASC		PVV

1		U1336SF		Generate a VISA PIN Verification Value (of a customer selected PIN)
1.0		F2ASC=FW	Command Code
1.1		F3ASC		PIN Block Key Type
1.2		Rkey		PIN Block Key
1.3		Rkey		PVK Pair
1.4		F16ASC		PIN Block
1.5		F2ASC		PIN Block Format Code
1.6		F12ASC		Account Number
1.6		F18ASC		Account Number
1.7		F1ASC		PVKI
1.8		Rexcl		Excluded PIN Table
1.9		F1ASC=%		Delimiter
1.10		F2ASC		LMK Identifier
1		U8SF		Generate a VISA PIN Verification Value (Response)
1.0		F2ASC=DH	Response Code
1.1		F2ASC		Error Code
1.2		F4ASC		PVV

1		U1194SF		Load the Excluded PIN Table
1.0		F2ASC=BM	Command Code
1.1		F2ASC		Excluded PIN Count
1.2		F2ASC		Excluded PIN Length
1.3		U1188ASC	Excluded PIN Table
1		U SF		Load the Excluded PIN Table (Response)
1.0		F2ASC=BN	Response Code
1.1		F2ASC		Error Code

# PIN Change Commands

1		U1415SF		Verify & Generate an IBM PIN Offset (of customer selected new PIN)
1.0		F2ASC=DU	Command Code
1.1		F3ASC		PIN Block Key Type
1.2		Rkey		PIN Block Key
1.3		Rkey		PVK
1.4		F16ASC		Current PIN Block
1.5		F2ASC		PIN Block Format Code
1.6		F2ASC		Check Length
1.7		F12ASC		Account Number
1.7		F18ASC		Account Number
1.8		Rdectab		Decimalization Table
1.9		Rpval		PIN Validation Data
1.10		F12ASC		Current Offset
1.11		F16ASC		New PIN Block
1.12		Rexcl		Excluded PIN Table
1.13		F1ASC=%		Delimiter
1.14		F2ASC		LMK Identifier
1		U14SF		Verify & Generate an IBM PIN Offset (Response)
1.0		F2ASC=DV	Response Code
1.1		F2ASC		Error Code
1.2		F12ASC		New Offset

1		U1356SF		Verify & Generate a VISA PVV (of a customer selected PIN)
1.0		F2ASC=CU	Command Code
1.1		F3ASC		PIN Block Key Type
1.2		Rkey		PIN Block Key
1.3		Rkey		PVK Pair
1.4		F16ASC		Current PIN Block
1.5		F2ASC		PIN Block Format Code
1.6		F12ASC		Account Number
1.6		F18ASC		Account Number
1.7		F1ASC		PVKI
1.8		F4ASC		Current PVV
1.9		F16ASC		New PIN Block
1.10		Rexcl		Excluded PIN Table
1.11		F1ASC=%		Delimiter
1.12		F2ASC		LMK Identifier
1		U8SF		Verify & Generate a VISA PVV (Response)
1.0		F2ASC=CV	Response Code
1.1		F2ASC		Error Code
1.2		F4ASC		New PVV

# PIN Verification Commands

1		U202SF		Verify a Terminal PIN Using the IBM Method
1.0		F2ASC=DA	Command Code
1.1		Rkey		TPK
1.2		Rkey		PVK
1.3		F2ASC=12	Maximum PIN Length
1.4		F16ASC		PIN Block
1.5		F2ASC		PIN Block Format Code
1.6		F2ASC		Check Length
1.7		F12ASC		Account Number
1.7		F18ASC		Account Number
1.8		Rdectab		Decimalization Table
1.9		Rpval		PIN Validation Data
1.10		F12ASC		Offset
1.11		F1ASC=%		Delimiter
1.12		F2ASC		LMK Identifier
1		U4SF		Verify a Terminal PIN Using the IBM Method (Response)
1.0		F2ASC=DB	Response Code
1.1		F2ASC		Error Code

1		U205SF		Verify an Interchange PIN Using the IBM Method
1.0		F2ASC=EA	Command Code
1.1		Rkey		ZPK
1.2		Rkey		PVK
1.3		F2ASC=12	Maximum PIN Length
1.4		F16ASC		PIN Block
1.5		F2ASC		PIN Block Format Code
1.6		F2ASC		Check Length
1.7		F12ASC		Account Number
1.7		F18ASC		Account Number
1.8		Rdectab		Decimalization Table
1.9		Rpval		PIN Validation Data
1.10		F12ASC		Offset
1.11		F1ASC=%		Delimiter
1.12		F2ASC		LMK Identifier
1		U4SF		Verify an Interchange PIN Using the IBM Method (Response)
1.0		F2ASC=EB	Response Code
1.1		F2ASC		Error Code

1		U116SF		Verify a Terminal PIN Using the Diebold Method
1.0		F2ASC=CG	Command Code
1.1		Rkey		TPK
1.2		F4SF		PVK
1.2.0		F1ASC=K		Index Flag
1.2.1		F3ASC		Table Pointer
1.3		F2ASC		Algorithm Number
1.4		F16ASC		PIN Block
1.5		F2ASC		PIN Block Format Code
1.6		F12ASC		Account Number
1.6		F18ASC		Account Number
1.7		F16ASC		Validation Data
1.8		F4ASC		Offset
1.9		F1ASC=%		Delimiter
1.10		F2ASC		LMK Identifier
1		U4SF		Verify a Terminal PIN Using the Diebold Method (Response)
1.0		F2ASC=CH	Response Code
1.1		F2ASC		Error Code

1		U116SF		Verify an Interchange PIN Using the Diebold Method
1.0		F2ASC=EG	Command Code
1.1		Rkey		ZPK
1.2		F4SF		PVK
1.2.0		F1ASC=K		Index Flag
1.2.1		F3ASC		Table Pointer
1.3		F2ASC		Algorithm Number
1.4		F16ASC		PIN Block
1.5		F2ASC		PIN Block Format Code
1.6		F12ASC		Account Number
1.6		F18ASC		Account Number
1.7		F16ASC		Validation Data
1.8		F4ASC		Offset
1.9		F1ASC=%		Delimiter
1.10		F2ASC		LMK Identifier
1		U4SF		Verify an Interchange PIN Using the Diebold Method (Response)
1.0		F2ASC=EH	Response Code
1.1		F2ASC		Error Code

1		U144SF		Verify a Terminal PIN Using the VISA Method
1.0		F2ASC=DC	Command Code
1.1		Rkey		TPK
1.2		Rkey		PVK Pair
1.3		F16ASC		PIN Block
1.4		F2ASC		PIN Block Format Code
1.5		F12ASC		Account Number
1.5		F18ASC		Account Number
1.6		F1ASC		PVKI
1.7		F4ASC		PVV
1.8		F1ASC=%		Delimiter
1.9		F2ASC		LMK Identifier
1		U4SF		Verify a Terminal PIN Using the VISA Method (Response)
1.0		F2ASC=DD	Response Code
1.1		F2ASC		Error Code

1		U144SF		Verify an Interchange PIN Using the VISA Method
1.0		F2ASC=EC	Command Code
1.1		Rkey		ZPK
1.2		Rkey		PVK Pair
1.3		F16ASC		PIN Block
1.4		F2ASC		PIN Block Format Code
1.5		F12ASC		Account Number
1.5		F18ASC		Account Number
1.6		F1ASC		PVKI
1.7		F4ASC		PVV
1.8		F1ASC=%		Delimiter
1.9		F2ASC		LMK Identifier
1		U4SF		Verify an Interchange PIN Using the VISA Method (Response)
1.0		F2ASC=ED	Response Code
1.1		F2ASC		Error Code

1		U123SF		Verify a Terminal PIN Using the Comparison Method
1.0		F2ASC=BC	Command Code
1.1		Rkey		TPK
1.2		F16ASC		PIN Block
1.3		F2ASC		PIN Block Format Code
1.4		F12ASC		Account Number
1.4		F18ASC		Account Number
1.5		Rpin		PIN
1.6		F1ASC=%		Delimiter
1.7		F2ASC		LMK Identifier
1		U4SF		Verify a Terminal PIN Using the Comparison Method (Response)
1.0		F2ASC=BD	Response Code
1.1		F2ASC		Error Code

1		U123SF		Verify an Interchange PIN PIN Using the Comparison Method
1.0		F2ASC=BE	Command Code
1.1		Rkey		ZPK
1.2		F16ASC		PIN Block
1.3		F2ASC		PIN Block Format Code
1.4		F12ASC		Account Number
1.4		F18ASC		Account Number
1.5		Rpin		PIN
1.6		F1ASC=%		Delimiter
1.7		F2ASC		LMK Identifier
1		U4SF		Verify an Interchange PIN PIN Using the Comparison Method (Response)
1.0		F2ASC=BF	Response Code
1.1		F2ASC		Error Code

# PIN Translation Commands

1		U135SF		Translate a PIN from One ZPK to Another
1.0		F2ASC=CC	Command Code
1.1		Rkey		Source ZPK
1.2		Rkey		Destination ZPK
1.3		F2ASC=12	Maximum PIN length
1.4		F16ASC		Source PIN block
1.5		F2ASC		Source PIN block format
1.6		F2ASC		Destination PIN block format
1.7		F12ASC		Account number
1.7		F18ASC		Account number
1.8		F1ASC=%		Delimiter
1.9		F2ASC		LMK Identifier
1		U16SF		Translate a PIN from One ZPK to Another (Response)
1.0		F2ASC=CD	Response Code
1.1		F2ASC		Error Code
1.2		F2ASC		PIN length
1.3		F16ASC		Destination PIN block
1.4		F2ASC		Destination PIN block format

1		U135SF		Translate a PIN from TPK to ZPK Encryption
1.0		F2ASC=CA	Command Code
1.1		Rkey		Source TPK
1.2		Rkey		Destination ZPK
1.3		F2ASC		Maximum PIN length
1.4		F16ASC		Source PIN block
1.5		F2ASC		Source PIN block format
1.6		F2ASC		Destination PIN block format
1.7		F12ASC		Account number
1.7		F18ASC		Account number
1.8		F1ASC=%		Delimiter
1.9		F2ASC		LMK Identifier
1		U16SF		Translate a PIN from TPK to ZPK Encryption (Response)
1.0		F2ASC=CB	Response Code
1.1		F2ASC		Error Code
1.2		F2ASC		PIN length
1.3		F16ASC		Destination PIN block
1.4		F2ASC		Destination PIN block format

1		U90SF		Translate a PIN from ZPK to LMK Encryption
1.0		F2ASC=JE	Command Code
1.1		Rkey		Source ZPK
1.2		F16ASC		PIN block
1.3		F2ASC		PIN block format code
1.4		F12ASC		Account number
1.4		F18ASC		Account number
1.5		F1ASC=%		Delimiter
1.6		F2ASC		LMK Identifier
1		U37SF		Translate a PIN from ZPK to LMK Encryption (Response)
1.0		F2ASC=JF	Response Code
1.1		F2ASC		Error Code
1.2		Rpin		PIN

1		U90SF		Translate a PIN from TPK to LMK Encryption
1.0		F2ASC=JC	Command Code
1.1		Rkey		Source TPK
1.2		F16ASC		PIN block
1.3		F2ASC		PIN block format code
1.4		F12ASC		Account number
1.4		F18ASC		Account number
1.5		F1ASC=%		Delimiter
1.6		F2ASC		LMK Identifier
1		U37SF		Translate a PIN from TPK to LMK Encryption (Response)
1.0		F2ASC=JD	Response Code
1.1		F2ASC		Error Code
1.2		Rpin		PIN

1		U107SF		Translate a PIN from LMK to ZPK Encryption
1.0		F2ASC=JG	Command Code
1.1		Rkey		Destination ZPK
1.2		F2ASC		PIN block format code
1.3		F12ASC		Account number
1.3		F18ASC		Account number
1.4		Rpin		PIN
1.5		F1ASC=%		Delimiter
1.6		F2ASC		LMK Identifier
1		U20SF		Translate a PIN from LMK to ZPK Encryption (Response)
1.0		F2ASC=JH	Response Code
1.1		F2ASC		Error Code
1.2		F16ASC		PIN Block

1		U33SF		Translate PIN Algorithm
1.0		F2ASC=BQ	Command Code
1.1		F12ASC		Account number
1.2		Rpin		PIN
1.3		F1ASC=%		Delimiter
1.4		F2ASC		LMK Identifier
1		U20SF		Translate PIN Algorithm (Response)
1.0		F2ASC=BR	Response Code
1.1		F2ASC		Error Code
1.2		Rpin		PIN

1		U74SF		Translate Account Number for LMK-encrypted PIN
1.0		F2ASC=QK	Command Code
1.1		Rpin		PIN
1.2		F12ASC		Old Account number
1.2		F18ASC		Old Account number
1.3		F12ASC		New Account number
1.3		F18ASC		New Account number
1.4		F1ASC=%		Delimiter
1.5		F2ASC		LMK Identifier
1		U37SF		Translate Account Number for LMK-encrypted PIN (Response)
1.0		F2ASC=QL	Response Code
1.1		F2ASC		Error Code
1.2		Rpin		PIN

# PIN Mailer Printing Commands

1		U2612SF		Print PIN/PIN and Solicitation Data
1.0		F2ASC=PE	Command Code
1.1		F1ASC		Document type
1.2		F12ASC		Account Number
1.3		Rpin		PIN
1.4		U2560SF		Print Fields
1.4.0		U255SF		Print Field 0
1.4.0.1		U255ASC		Print Field data
1.4.1		U256SF		Print Field 1
1.4.1.0		F1ASC=;		Delimiter
1.4.1.1		U255ASC		Print Field data
1.4.2		R1.4.1		Print Field 2
1.4.3		R1.4.1		Print Field 3
1.4.4		R1.4.1		Print Field 4
1.4.5		R1.4.1		Print Field 5
1.4.6		R1.4.1		Print Field 6
1.4.7		R1.4.1		Print Field 7
1.4.8		R1.4.1		Print Field 8
1.4.9		R1.4.1		Print Field 9
1.5		F1ASC=~		Delimiter
1.6		F1ASC=%		Delimiter
1.7		F2ASC		LMK Identifier
1		U32SF		Print PIN/PIN and Solicitation Data (Response)
1.0		F2ASC=PF	Response Code (before printing)
1.1		F2ASC		Error Code
1.2		U28ASC		PIN check value and Reference number check value
1		F4SF		Print PIN/PIN and Solicitation Data (Response)
1.0		F2ASC=PZ	Response Code (after printing)
1.1		F2ASC		Error Code

1		U2579SF		Print a PIN Solicitation Mailer
1.0		F2ASC=OA	Command Code
1.1		F1ASC		Document type
1.2		F12ASC		Account Number
1.4		U2560SF		Print Fields
1.4.0		U255SF		Print Field 0
1.4.0.1		U255ASC		Print Field data
1.4.1		U256SF		Print Field 1
1.4.1.0		F1ASC=;		Delimiter
1.4.1.1		U255ASC		Print Field data
1.4.2		R1.4.1		Print Field 2
1.4.3		R1.4.1		Print Field 3
1.4.4		R1.4.1		Print Field 4
1.4.5		R1.4.1		Print Field 5
1.4.6		R1.4.1		Print Field 6
1.4.7		R1.4.1		Print Field 7
1.4.8		R1.4.1		Print Field 8
1.4.9		R1.4.1		Print Field 9
1.5		F1ASC=~		Delimiter
1.6		F1ASC=%		Delimiter
1.7		F2ASC		LMK Identifier
1		F16SF		Print a PIN Solicitation Mailer (Response)
1.0		F2ASC=OB	Response Code (before printing)
1.1		F2ASC		Error Code
1.2		F12ASC		PIN check value and Reference number check value
1		F4SF		Print a PIN Solicitation Mailer (Response)
1.0		F2ASC=OZ	Response Code (after printing)
1.1		F2ASC		Error Code

1		U78SF		Verify PIN/PIN and Solicitation Mailer Cryptography
1.0		F2ASC=PG	Command Code
1.1		F12ASC		Account Number
1.2		Rpin		PIN
1.3		U28ASC		PIN check value and Reference number check value
1.4		F1ASC=%		Delimiter
1.5		F2ASC		LMK Identifier
1		F4SF		Verify PIN/PIN and Solicitation Mailer Cryptography (Response)
1.0		F2ASC=PH	Response Code
1.1		F2ASC		Error Code

1		U29SF		Verify Solicitation Mailer Cryptography
1.0		F2ASC=RC	Command Code
1.1		F12ASC		Account Number
1.3		F12ASC		Reference number check value
1.4		F1ASC=%		Delimiter
1.5		F2ASC		LMK Identifier
1		F4SF		Verify Solicitation Mailer Cryptography (Response)
1.0		F2ASC=RD	Response Code
1.1		F2ASC		Error Code

1		U2599SF		Print TMK Mailer
1.0		F2ASC=TA	Command Code
1.1		Rkey		TMK
1.2		U2560SF		Print Fields
1.2.0		U255SF		Print Field 0
1.2.0.0		U255ASC		Print Field data
1.2.0.1		F1ASC=;		Delimiter
1.2.1		R1.2.0		Print Field 1
1.2.2		R1.2.0		Print Field 2
1.2.3		R1.2.0		Print Field 3
1.2.4		R1.2.0		Print Field 4
1.2.5		R1.2.0		Print Field 5
1.2.6		R1.2.0		Print Field 6
1.2.7		R1.2.0		Print Field 7
1.2.8		R1.2.0		Print Field 8
1.2.9		R1.2.0		Print Field 9
1.3		F1ASC=~		Delimiter
1.4		F1ASC=%		Delimiter
1.5		F2ASC		LMK Identifier
1		F4SF		Print TMK Mailer (Response)
1.0		F2ASC=TB	Response Code (before printing)
1.1		F2ASC		Error Code
1		F4SF		Print TMK Mailer (Response)
1.0		F2ASC=TZ	Response Code (after printing)
1.1		F2ASC		Error Code

# PIN Solicitation Data Processing Commands

1		U627SF		Load Solicitation Data to User Storage
1.0		F2ASC=QA	Command Code (Batch load)
1.0		F2ASC=QC	Command Code (Final load)
1.1		U625SF		Solicitation Data
1.1.0		U25SF		Solicitation Data 0
1.1.0.0		U24AS/c		data
1.1.0.1		F1ASC=;		delimiter
1.1.1		R1.1.0		Solicitation Data 1
1.1.2		R1.1.0		Solicitation Data 2
1.1.3		R1.1.0		Solicitation Data 3
1.1.4		R1.1.0		Solicitation Data 4
1.1.5		R1.1.0		Solicitation Data 5
1.1.6		R1.1.0		Solicitation Data 6
1.1.7		R1.1.0		Solicitation Data 7
1.1.8		R1.1.0		Solicitation Data 8
1.1.9		R1.1.0		Solicitation Data 9
1.1.10		R1.1.0		Solicitation Data 10
1.1.11		R1.1.0		Solicitation Data 11
1.1.12		R1.1.0		Solicitation Data 12
1.1.13		R1.1.0		Solicitation Data 13
1.1.14		R1.1.0		Solicitation Data 14
1.1.15		R1.1.0		Solicitation Data 15
1.1.16		R1.1.0		Solicitation Data 16
1.1.17		R1.1.0		Solicitation Data 17
1.1.18		R1.1.0		Solicitation Data 18
1.1.19		R1.1.0		Solicitation Data 19
1.1.20		R1.1.0		Solicitation Data 20
1.1.21		R1.1.0		Solicitation Data 21
1.1.22		R1.1.0		Solicitation Data 22
1.1.23		R1.1.0		Solicitation Data 23
1.1.24		R1.1.0		Solicitation Data 24
1		F4SF		Load Solicitation Data to User Storage (Response)
1.0		F2ASC=QB	Response Code (Batch load)
1.1		F2ASC		Error Code
1		U27724SF	Load Solicitation Data to User Storage (Response)
1.0		F2ASC=QD	Response Code (Final load)
1.1		F2ASC		Error Code
1.2		U27720ASC	Processed Data

# Clear PIN Commands







1		U78SF		Generate a VISA CVV
1.0		F2ASC=CW	Command Code
1.1		Rkey		CVK A / B
1.2		U19ASC		Primary account number
1.3		F1ASC=;		Delimiter
1.4		F4ASC		Expiration date
1.5		F3ASC		Service code
1		U7SF		Generate a VISA CVV (Response)
1.0		F2ASC=CX	Response code
1.1		F2ASC		Error code
1.2		F3ASC		CVV

1		U81SF		Verify a VISA CVV
1.0		F2ASC=CY	Command Code
1.1		Rkey		CVK A / B
1.2		F3ASC		CVV
1.3		U19ASC		Primary account number
1.4		F1ASC=;		Delimiter
1.5		F4ASC		Expiration date
1.6		F3ASC		Service code
1		F4SF		Verify a VISA CVV (Response)
1.0		F2ASC=CZ	Response code
1.1		F2ASC		Error code




#1		U1019SF		Unknown command
#1.0		F2ASC		Command Code
#1.1		U1017HEX	Data
2		F1HEX=19	Trailer control character
3		U32ASC		Message Trailer
