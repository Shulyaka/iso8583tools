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

1		U82SF		Translate a PIN from One ZPK to Another
1.0		F2ASC=CC	Command code
1.1		Rkey		Source ZPK
1.2		Rkey		Destination ZPK
1.3		F2ASC		Maximum PIN length
1.4		F8HEX		Source PIN block
1.5		F2ASC		Source PIN block format
1.6		F2ASC		Destination PIN block format
1.7		F16ASC		Account number
1		U16SF		Translate a PIN from One ZPK to Another (Response)
1.0		F2ASC=CD	Response code
1.1		F2ASC		Error code
1.2		F2ASC		PIN length
1.3		F8HEX		Destination PIN block
1.4		F2ASC		Destination PIN block format

1		U46SF		Generate a VISA CVV
1.0		F2ASC=CW	Command code
1.1		Rkey		CVK A / B
1.2		U19ASC		Primary account number
1.3		F1ASC=;		Delimiter
1.4		F4ASC		Expiration date
1.5		F3ASC		Service code
1		U7SF		Generate a VISA CVV (Response)
1.0		F2ASC=CX	Response code
1.1		F2ASC		Error code
1.2		F3ASC		CVV

1		U49SF		Verify a VISA CVV
1.0		F2ASC=CY	Command code
1.1		Rkey		CVK A / B
1.2		F3ASC		CVV
1.3		U19ASC		Primary account number
1.4		F1ASC=;		Delimiter
1.5		F4ASC		Expiration date
1.6		F3ASC		Service code
1		F4SF		Verify a VISA CVV (Response)
1.0		F2ASC=CZ	Response code
1.1		F2ASC		Error code




1		U1019SF		Unknown command
1.0		F2ASC		Command Code
1.1		U1017HEX	Data
2		F1HEX=19	Trailer control character
3		U32ASC		Message Trailer
