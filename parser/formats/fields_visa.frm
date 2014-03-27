# A visa (from the Latin charta visa, lit. "paper that has been seen") is a document showing that a person is authorized to enter or leave the territory for which it was issued, subject to permission of an immigration official at the time of actual entry.

# This is just an example, not a real Visa header
#Number		Format		Description
message 	BB800SF		Visa Message (with header)
0		BI22SF		Visa Header
0.2		F2BCD		Header Format
0.3		F2BCD		Text format
0.4		F2HEX		Total message length
0.5		F6BCD		Destination ID
0.6		F6BCD		Source ID
0.7		F8BITSTR	Round-Trip Control Info
0.8		F16BITSTR	BASE I Flags
0.9		F24BITSTR	Message Status Flags
0.10		F1HEX		Batch Number
0.11		F3HEX		Reserved for Visa International Use
0.12		F2BCD		User Info
0.13		F16BITMAP	Bitmap
0.14		F4BCD		Reject Data Group
1		U788SF		Visa Message
1.0		F4BCD		Message type
1.1		ISOBITMAP	Message Bit Map
1.2		B19BCD		Primary Account Number
1.3		F3SF		Processing code
1.3.1		F2BCD		transaction type
1.3.2		F2BCD		account type from
1.3.3		F2BCD		account type to
1.4		F12BCD		Amount, Transaction
1.5		F12BCD		Amount, Settlement
1.6		F12BCD		Amount, cardholder billing
1.7		F10BCD		Transmission date and time
1.9		F8BCDSF		Conversion Rate, Settlement
1.9.0		F1ASC		exponent
1.9.1		F7ASC		mantissa
1.10		F8BCDSF		Converdion rate, cardholder billing
1.10.0		F1ASC		exponent
1.10.1		F7ASC		mantissa
1.11		F6BCD		System Trace Audit Number
1.12		F6BCD		Time, local transaction
1.13		F4BCD		Date, local transaction
1.14		F4BCD		Date, expiration
1.15		F4BCD		Date, Settlement
1.16		F4BCD		Date, Conversion
1.18		F4BCD		Merchant type
1.19		F3BCD		Acquiring institution country code
1.20		F3BCD		PAN Extended, Country Code
1.22		F3BCDSF		Point-of-Service Entry Mode Code
1.22.1		F2ASC		PAN/date entry mode
1.22.2		F1ASC		PAN entry capability
1.23		F3BCD		Card Sequence Number
1.25		F2BCD		Point-of-Service Condition Code
1.26		F2BCD		Point-of-Service PIN Capture Code
1.28		F9EBCDIC	Amount, Transaction Fee
1.32		B11BCD		Acquiring Institution Identification Code
1.33		B11BCD		Forwarding Institution Identification Code
1.35		B37BCD		Track 2 Data
1.37		F12EBCDIC	Retrieval Reference Number
1.38		F6EBCDIC	Authorization Identification Response
1.39		F2EBCDIC	Response Code
1.41		F8EBCDIC	Card Acceptor Terminal Identification
1.42		F15EBCDIC	Card Acceptor Identification Code
1.43		F40SF		Card Acceptor Name/Location
1.43.1		F25EBCDIC	card acceptor name or ATM location
1.43.2		F13EBCDIC	city name
1.43.3		F2EBCDIC	country code
1.44		B25SF		Additional Response Data
1.44.1		F1EBCDIC	response source/reason code
1.44.2		F1EBCDIC	address verification result code
1.44.3		F1EBCDIC        reserved
1.44.4		F1EBCDIC        card product type
1.44.5		F1EBCDIC        CVV/iCVV results code
1.44.6		F2EBCDIC        PACM diversion level
1.44.7		F1EBCDIC        PACM diversion reason code
1.44.8		F1EBCDIC        card authentication result code
1.44.9		F1EBCDIC        Reserved
1.44.10		F1EBCDIC        CVV2 results code
1.44.11		F2EBCDIC        original response code
1.44.12		F1EBCDIC        check settlement code (U.S. only)
1.44.13		F1EBCDIC        CAVV results code
1.44.14		F4EBCDIC        response reason code
1.45		B76EBCDIC	Track 1 Data
1.48		B255EBCDIC	Additional Data—Private
1.49		F3BCD		Currency Code, Transaction
1.50		F3BCD		Currency Code, Settlement
1.51		F3BCD		Currency Code, Cardholder Billing
1.52		F8HEX		Personal Identification Number (PIN) Data
1.53		F8SF		Security-Related Control Information
1.53.1		F2BCD		security format code
1.53.2		F2BCD           algorithm ID
1.53.3		F2BCD           PIN block format code
1.53.4		F2BCD           zone key index
1.53.5		F2BCD           PIN Data Type
1.53.6		F6BCD           Visa Reserved
1.54		B120SF		Additional Amounts
1.54.0		F20SF		amount 0
1.54.0.1	F2EBCDIC	account type
1.54.0.2	F2EBCDIC	amount type
1.54.0.3	F3EBCDIC	currency code
1.54.0.4	F1EBCDIC	amount, sign
1.54.0.5	F12EBCDIC	amount
1.54.1		F20SF		amount 1
1.54.1.1	F2EBCDIC	account type
1.54.1.2	F2EBCDIC	amount type
1.54.1.3	F3EBCDIC	currency code
1.54.1.4	F1EBCDIC	amount, sign
1.54.1.5	F12EBCDIC	amount
1.54.2		F20SF		amount 2
1.54.2.1	F2EBCDIC	account type
1.54.2.2	F2EBCDIC	amount type
1.54.2.3	F3EBCDIC	currency code
1.54.2.4	F1EBCDIC	amount, sign
1.54.2.5	F12EBCDIC	amount
1.54.3		F20SF		amount 3
1.54.3.1	F2EBCDIC	account type
1.54.3.2	F2EBCDIC	amount type
1.54.3.3	F3EBCDIC	currency code
1.54.3.4	F1EBCDIC	amount, sign
1.54.3.5	F12EBCDIC	amount
1.54.4		F20SF		amount 4
1.54.4.1	F2EBCDIC	account type
1.54.4.2	F2EBCDIC	amount type
1.54.4.3	F3EBCDIC	currency code
1.54.4.4	F1EBCDIC	amount, sign
1.54.4.5	F12EBCDIC	amount
1.54.5		F20SF		amount 5
1.54.5.1	F2EBCDIC	account type
1.54.5.2	F2EBCDIC	amount type
1.54.5.3	F3EBCDIC	currency code
1.54.5.4	F1EBCDIC	amount, sign
1.54.5.5	F12EBCDIC	amount
1.55		B255TLVDSBCD	Integrated Circuit Card (ICC)-Related Data
1.55.00         BB252TLV2	Chip Card TLV data elements (Usage 2)
1.55.00.*	B250HEX		Chip data tag
1.55.01		BB252TLVEMV	Chip Card TLV data elements (VSDC)
1.55.01.*	M250HEX		Chip data tag
1.59		B14EBCDIC	National Point-of-Service Geographic Data
1.60		B12BCDSF	Additional POS Information
1.60.1		F1ASC		terminal type
1.60.2		F1ASC		terminal entry capability
1.60.3		F1ASC		chip condition code
1.60.4		F1ASC		special condition indicator—existing debt
1.60.5		F2ASC		not applicable
1.60.6		F1ASC		transaction indicator
1.60.7		F1ASC		card authentication reliability indicator
1.60.8		F2ASC		mail/phone/electronic commerce and payment indicator
1.60.9		F1ASC		not applicable (SMS only)
1.60.10		F1ASC		partial authorization indicator
1.61		B36BCD		Other Amounts
1.62		B255SF		Custom Payment Service Fields (bitmap format)
1.62.0		F64BITMAP	Field 62 Bitmap
1.62.1		F1EBCDIC	Authorization Characteristics Indicator
1.62.2		F15BCD		Transaction Identifier
1.62.3		F4EBCDIC	Validation Code
1.62.4		F1EBCDIC	Market-Specific Data Identifier
1.62.5		F2BCD		Duration
1.62.6		F1EBCDIC	Prestigious Property Indicator
1.62.17		F15EBCDIC	MC Interchange Compliance
1.62.20		F10BCD		Merchant Verification Value
1.62.21		F4EBCDIC	Online Risk Assessment Risk Score and Reason Codes
1.62.22		F6EBCDIC	Online Risk Assessment Condition Codes
1.62.23		F2EBCDIC	Card-Level Results
1.62.24		F6EBCDIC	Program Identifier
1.62		B13SF		Custom Payment Service Fields (fixed format)
1.62.1		F1EBCDIC	Authorization Characteristics Indicator
1.62.2		F15BCD		Transaction Identifier
1.62.3		F4EBCDIC	Validation Code
1.63		B255SF		V.I.P. Private-Use Field
1.63.0		F24BITMAP	Bitmap
1.63.1		F4BCD		Network ID
1.63.2		F4BCD		Time (Preauth Time Limit)
1.63.3		F4BCD		Message Reason Code
1.63.4		F4BCD		STIP/Switch Reason Code
1.63.5		F6BCD		n/a
1.63.6		F7EBCDIC	Chargeback Reduction/BASE II Flags
1.63.7		F16HEX		n/a
1.63.8		F8BCD		n/a
1.63.9		F28HEX		Fraud Data
1.63.10		F26HEX		n/a
1.63.11		F1EBCDIC	Reimbursement Attribute
1.63.12		F30EBCDIC	Sharing Group Code
1.63.13		F6BCD		Decimal Positions Indicator
1.63.14		F36EBCDIC	Issuer Currency Conversion Data
1.63.15		F9EBCDIC	n/a, reserved for future use
1.63.16		F6BCD		n/a
1.63.18		F2BCD		n/a
1.63.19		F3EBCDIC	Fee Program indicator
1.63.20		F2BCD		n/a
1.63.21		F1EBCDIC	Charge Indicator
1.66		F1BCD		Settlement Code
1.68		F3BCD		Receiving Institution Country Code
1.70		F3BCD		Network Management Informatioin Code
1.73		F6BCD		Date, Action
1.90		F42BCDSF	Original Data Elements
1.90.1		F4ASC		original message type
1.90.2		F6ASC		original trace number
1.90.3		F10ASC		original transmission date/time
1.90.4		F11ASC		original acquirer ID
1.90.5		F11ASC		original forwarding institution ID
1.91		F1EBCDIC	File Update Code
1.92		F2EBCDIC	File Security Code
1.95		F42SF		Replacement amounts
1.95.1		F12EBCDIC	actual amount, transaction
1.95.2		F12EBCDIC	unused
1.95.3		F9EBCDIC	unused
1.95.4		F9EBCDIC	unused
1.100		B11BCD		Receiving Institution Identification Code
1.101		B17EBCDIC	File Name
1.102		B28EBCDIC	Account Identification 1
1.103		B28EBCDIC	Account Identification 2
1.104		B255TLV1HEX	Transaction-Specific Data
1.104.*		BB252TLV1HEX	dataset ID
1.104.*.*	B250EBCDIC	data
1.116		B255TLV1HEX	Card Issuer Reference Data
1.116.*		BB252TLV1HEX	dataset ID
1.116.*.*	B250EBCDIC	data
1.117		B255SF		National Use
1.117.1		F3EBCDIC	country code
1.117.2		U252EBCDIC	data
1.118		B255SF		Intra-Country Data
1.118.1		F3EBCDIC	country code
1.118.2		U252EBCDIC	data
1.120		B4HEX		Original Message Type ID
1.121		B11EBCDIC	Issuing Institution Identification Code
#1.123		B255TLV1HEX	Verification Data
#1.123.*		BB252TLV1HEX	dataset ID
#1.123.*.*	B250EBCDIC	Verification Data TLV element
1.123		B30SF		Verification Data
1.123.0		F9EBCDIC	postal code
1.123.1		U20EBCDIC	cardholder street address
1.125		B255EBCDIC	Supporting Information
1.126		B255SF		Visa Private-Use Fields
1.126.0		F64BITMAP	Field126 Bitmap
1.126.6		F17SF		Cardholder Certificate Serial Number
1.126.6.0	F1HEX		number of significant digits
1.126.6.1	F16HEX		cardholder certificate serial number
1.126.7		F17SF		Merchant Certificate Serial Number
1.126.7.0	F1HEX		number of significatn digits
1.126.7.1	F16HEX		merchant certificate serial number
1.126.8		F20HEX		Transaction ID (XID)
1.126.9		F20SF		TransStain/CAVV Data
1.126.9.1	F2BCD		3-D Secure Authentication Results Code
1.126.9.2	F2BCD		Second Factor Authentication Code
1.126.9.3	F2BCD		CAVV Key Indicator
1.126.9.4	F3BCD		CAVV Value
1.126.9.5	F4BCD		CAVV Unpredictable Number
1.126.9.6	F16BCD		Authentication Tracking Number
1.126.9.7 	F2BCDSF		Version and Authentication Action
1.126.9.7.0	F1ASC		Version
1.126.9.7.1	F1ASC		Authentication Value
1.126.9.8	F4HEX		IP Address in Hex Format
1.126.10	F6SF		CVV2 Authorization Request Data
1.126.10.1	F1EBCDIC	Presence Indicator
1.126.10.2	F1EBCDIC	Response Type
1.126.10.3	F4EBCDIC	CVV2 Value
1.126.12	F24BITSTR	Service Indicators
1.126.13	F1EBCDIC	POS Environment
1.126.14	F1EBCDIC	Payment Guarantee Option
1.126.15	F1EBCDIC	MC UCAF Collection Indicator
1.126.16	B32EBCDIC	MC UCAF Field
1.126.18	F12SF		Agent Unique Account Result
1.126.18.0	F1HEX		dataset ID
1.126.18.1	F5EBCDIC
1.126.18.2	F48BITSTR
1.127		B255EBCDIC	File Record(s): Action and Data
1.130		F24BITSTR	Terminal Capability Profile
1.131		F40BITSTR	Terminal Verification Results (TVR)
1.132		F4HEX		Unpredictable Number
1.133		F8EBCDIC	Terminal Serial Number
1.134		B255HEX		Visa Discretionary Data
1.135		B15HEX		Issuer Discretionary Data
1.136		F8HEX		Cryptogram
1.137		F2HEX		Application Transaction Counter
1.138		F16BITSTR	Application Interchange Profile
1.139		F10SF		ARPC Response Cryptogram and Code
1.139.1		F8HEX		ARPC Cryptogram
1.139.2		F2EBCDIC	ARPC response code
1.140		B255SF		Issuer Authentication Data
1.140.1		F16HEX		IAD
1.140.2		U239HEX		Reserved
1.142		B255HEX		Issuer Script
1.143		B20HEX		Issuer Script Result
1.144		F2BCD		Cryptogram Transaction Type
1.145		F3BCD		Terminal Country Code
1.146		F6BCD		Terminal Transaction Date
1.147		F12BCD		Cryptogram Amount
1.148		F3BCD		Cryptogram Currency Code
1.149		F12BCD		Cryptogram Cashback Amount
1.152		F8HEX		Secondary PIN Block
