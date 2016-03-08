# A visa (from the Latin charta visa, lit. "paper that has been seen") is a document showing that a person is authorized to enter or leave the territory for which it was issued, subject to permission of an immigration official at the time of actual entry.

# This is just an example, not a real Visa header
#Number		Format		Description
message 	BB800-2SF	Visa Message (with header)
0		F4HEX		Visa Message Length Header B3 B4
1		BI22SF		Visa Header
1.2		F2BCD		Header Format
1.3		F2BCD		Text format
1.4		F4HEX		Total message length
1.5		F6BCD		Destination ID
1.6		F6BCD		Source ID
1.7		F8BITSTR	Round-Trip Control Info
1.8		F16BITSTR	BASE I Flags
1.9		F24BITSTR	Message Status Flags
1.10		F2HEX		Batch Number
1.11		F6HEX		Reserved for Visa International Use
1.12		F2BCD		User Info
1.13		F16BITMAP	Bitmap
1.14		F4BCD		Reject Data Group
2		U788SF		Visa Message
2.0		F4BCD		Message type
2.1		ISOBITMAP	Message Bit Map
2.2		B19BCD		Primary Account Number
2.3		F3SF		Processing code
2.3.1		F2BCD		transaction type
2.3.2		F2BCD		account type from
2.3.3		F2BCD		account type to
2.4		F12BCD		Amount, Transaction
2.5		F12BCD		Amount, Settlement
2.6		F12BCD		Amount, Cardholder Billing
2.7		F10BCD		Transmission date and time
2.9		F8BCDSF		Conversion Rate, Settlement
2.9.0		F1ASC		exponent
2.9.1		F7ASC		mantissa
2.10		F8BCDSF		Converdion rate, cardholder billing
2.10.0		F1ASC		exponent
2.10.1		F7ASC		mantissa
2.11		F6BCD		System Trace Audit Number
2.12		F6BCD		Time, local transaction
2.13		F4BCD		Date, local transaction
2.14		F4BCD		Date, expiration
2.15		F4BCD		Date, Settlement
2.16		F4BCD		Date, Conversion
2.18		F4BCD		Merchant type
2.19		F3BCD		Acquiring institution country code
2.20		F3BCD		PAN Extended, Country Code
2.22		F3BCDSF		Point-of-Service Entry Mode Code
2.22.1		F2ASC		PAN/date entry mode
2.22.2		F1ASC		PAN entry capability
2.23		F3BCD		Card Sequence Number
2.25		F2BCD		Point-of-Service Condition Code
2.26		F2BCD		Point-of-Service PIN Capture Code
2.28		F9EBCDIC	Amount, Transaction Fee
2.32		B11BCD		Acquiring Institution Identification Code
2.33		B11BCD		Forwarding Institution Identification Code
2.35		B37BCD		Track 2 Data
2.37		F12EBCDIC	Retrieval Reference Number
2.38		F6EBCDIC	Authorization Identification Response
2.39		F2EBCDIC	Response Code
2.41		F8EBCDIC	Card Acceptor Terminal Identification
2.42		F15EBCDIC	Card Acceptor Identification Code
2.43		F40SF		Card Acceptor Name/Location
2.43.1		F25EBCDIC	card acceptor name or ATM location
2.43.2		F13EBCDIC	city name
2.43.3		F2EBCDIC	country code
2.44		B25SF		Additional Response Data
2.44.1		F1EBCDIC	response source/reason code
2.44.2		F1EBCDIC	address verification result code
2.44.3		F1EBCDIC        reserved
2.44.4		F1EBCDIC        card product type
2.44.5		F1EBCDIC        CVV/iCVV results code
2.44.6		F2EBCDIC        PACM diversion level
2.44.7		F1EBCDIC        PACM diversion reason code
2.44.8		F1EBCDIC        card authentication result code
2.44.9		F1EBCDIC        Reserved
2.44.10		F1EBCDIC        CVV2 results code
2.44.11		F2EBCDIC        original response code
2.44.12		F1EBCDIC        check settlement code (U.S. only)
2.44.13		F1EBCDIC        CAVV results code
2.44.14		F4EBCDIC        response reason code
2.45		B76EBCDIC	Track 1 Data
2.46		B252SF		Amounts, Fees
2.46.1		F36SF		Fee Amount 1
2.46.1.1	F2EBCDIC	fee type
2.46.1.2	F3EBCDIC	currency code
2.46.1.3	F1EBCDIC	minor unit
2.46.1.4	F2EBCDIC	sign
2.46.1.5	F8EBCDIC	value
2.46.1.6	F22EBCDIC	unused
2.46.2		R2.46.1		Fee Amount 2
2.46.3		R2.46.1		Fee Amount 3
2.46.4		R2.46.1		Fee Amount 4
2.46.5		R2.46.1		Fee Amount 5
2.46.6		R2.46.1		Fee Amount 6
2.46.7		R2.46.1		Fee Amount 7
2.48		B4BCD		Usage 1b-Error Codes in 0312 Responses
2.48		B2EBCDIC	Usage 1c-Cardholder Maintenance File Reject Codes
2.48		B255SF		Additional Data—Private
2.48.0		F1EBCDIC	application identifier
2.48.1		U254EBCDIC	data
2.49		F3BCD		Currency Code, Transaction
2.50		F3BCD		Currency Code, Settlement
2.51		F3BCD		Currency Code, Cardholder Billing
2.52		F16HEX		Personal Identification Number (PIN) Data
2.53		F8SF		Security-Related Control Information
2.53.1		F2BCD		security format code
2.53.2		F2BCD           algorithm ID
2.53.3		F2BCD           PIN block format code
2.53.4		F2BCD           zone key index
2.53.5		F2BCD           PIN Data Type
2.53.6		F6BCD           Visa Reserved
2.54		B120SF		Additional Amounts
2.54.0		F20SF		amount 0
2.54.0.1	F2EBCDIC	account type
2.54.0.2	F2EBCDIC	amount type
2.54.0.3	F3EBCDIC	currency code
2.54.0.4	F1EBCDIC	amount, sign
2.54.0.5	F12EBCDIC	amount
2.54.1		R2.54.0		amount 1
2.54.2		R2.54.0		amount 2
2.54.3		R2.54.0		amount 3
2.54.4		R2.54.0		amount 4
2.54.5		R2.54.0		amount 5
2.55		B255TLV1BCD	Integrated Circuit Card (ICC)-Related Data
2.55.00         BB251TLV2BIN	Chip Card TLV data elements (Usage 2)
2.55.00.*	B500HEX		Chip data tag
2.55.01		BB251TLVBER	Chip Card TLV data elements (VSDC)
2.55.01.*	M500HEX		Chip data tag
2.59		B14EBCDIC	National Point-of-Service Geographic Data
2.60		B12BCDSF	Additional POS Information
2.60.1		F1ASC		terminal type
2.60.2		F1ASC		terminal entry capability
2.60.3		F1ASC		chip condition code
2.60.4		F1ASC		special condition indicator—existing debt
2.60.5		F2ASC		not applicable
2.60.6		F1ASC		transaction indicator
2.60.7		F1ASC		card authentication reliability indicator
2.60.8		F2ASC		mail/phone/electronic commerce and payment indicator
2.60.9		F1ASC		not applicable (SMS only)
2.60.10		F1ASC		partial authorization indicator
2.61		B36SF		Other Amounts
2.61.1		F12BCD		amount due
2.61.2		F12BCD		cardholder billing
2.61.3		F12BCD		replacement billing
2.62		B255SF		Custom Payment Service Fields (bitmap format)
2.62.0		F64BITMAP	Field 62 Bitmap
2.62.1		F1EBCDIC	Authorization Characteristics Indicator
2.62.2		F15BCD		Transaction Identifier
2.62.3		F4EBCDIC	Validation Code
2.62.4		F1EBCDIC	Market-Specific Data Identifier
2.62.5		F2BCD		Duration
2.62.6		F1EBCDIC	Prestigious Property Indicator
2.62.11		F2BCD		Multiple Clearing Sequence Number
2.62.12		F2BCD		Multiple Clearing Sequence Count
2.62.16		F2EBCDIC	Chargeback Rights Indicator
2.62.17		F15EBCDIC	MC Interchange Compliance
2.62.20		F10BCD		Merchant Verification Value
2.62.21		F4EBCDIC	Online Risk Assessment Risk Score and Reason Codes
2.62.22		F6EBCDIC	Online Risk Assessment Condition Codes
2.62.23		F2EBCDIC	Card-Level Results
2.62.24		F6EBCDIC	Program Identifier
2.62.25		F1EBCDIC	Spend Qualified Indicator
2.62		B13SF		Custom Payment Service Fields (fixed format)
2.62.1		F1EBCDIC	Authorization Characteristics Indicator
2.62.2		F15BCD		Transaction Identifier
2.62.3		F4EBCDIC	Validation Code
2.63		B255SF		V.I.P. Private-Use Field
2.63.0		F24BITMAP	Bitmap
2.63.1		F4BCD		Network ID
2.63.2		F4BCD		Time (Preauth Time Limit)
2.63.3		F4BCD		Message Reason Code
2.63.4		F4BCD		STIP/Switch Reason Code
2.63.5		F6BCD		n/a
2.63.6		F7SF		Chargeback Reduction/BASE II Flags
2.63.6.1	F1EBCDIC	floor limit indicator
2.63.6.2	F1EBCDIC	CRB indicator
2.63.6.3	F1EBCDIC	STIP indicator
2.63.6.4	F1EBCDIC	MOTO/ECI indicator
2.63.6.5	F1EBCDIC	special chargeback indicator
2.63.6.6	F2SF		special condition indicator
2.63.6.6.0	F1EBCDIC	risk indication
2.63.6.6.1	F1EBCDIC	merchant indication
2.63.7		F32HEX		n/a
2.63.8		F8BCD		n/a
2.63.9		F14SF		Fraud Data
2.63.9.0	F1EBCDIC	fraud type
2.63.9.1	F1EBCDIC	fraud notification code
2.63.9.2	F1EBCDIC	check fraud indicator (POS only)
2.63.9.3	F11EBCDIC	reserved
2.63.10		F52HEX		n/a
2.63.11		F1EBCDIC	Reimbursement Attribute
2.63.12		F30EBCDIC	Sharing Group Code
2.63.13		F3SF		Decimal Positions Indicator
2.63.13.1	F2BCD		transaction amounts decimal positions
2.63.13.2	F2BCD		settlement amounts decimal positions
2.63.13.3	F2BCD		cardholder amounts decimal positions
2.63.14		F36SF		Issuer Currency Conversion Data
2.63.14.0	F9EBCDIC	reserved
2.63.14.1	F9EBCDIC	reserved
2.63.14.2	F9EBCDIC	reserved
2.63.14.3	F9SF		Cardholder Billing Amount, Optional Issuer Fee
2.63.14.3.0	F1EBCDIC	prefix
2.63.14.3.1	F8EBCDIC	amount
2.63.15		F9EBCDIC	n/a, reserved for future use
2.63.16		F6BCD		n/a
2.63.18		F2BCD		n/a
2.63.19		F3EBCDIC	Fee Program indicator
2.63.20		F2BCD		n/a
2.63.21		F1EBCDIC	Charge Indicator
2.66		F1BCD		Settlement Code
2.68		F3BCD		Receiving Institution Country Code
2.69		F3BCD		Settlement Institution Country Code
2.70		F3BCD		Network Management Informatioin Code
2.73		F6BCD		Date, Action
2.74		F10BCD		Credits, Number
2.75		F10BCD		Credits, Reversal Number
2.76		F10BCD		Debits, Number
2.77		F10BCD		Debits, Reversal Number
2.86		F16BCD		Credits, Amount
2.87		F16BCD		Credits, Reversal Amount
2.88		F16BCD		Debits, Amount
2.89		F16BCD		Debits, Reversal Amount
2.90		F42BCDSF	Original Data Elements
2.90.1		F4ASC		original message type
2.90.2		F6ASC		original trace number
2.90.3		F10ASC		original transmission date/time
2.90.4		F11ASC		original acquirer ID
2.90.5		F11ASC		original forwarding institution ID
2.91		F1EBCDIC	File Update Code
2.92		F2EBCDIC	File Security Code
2.95		F42SF		Replacement amounts
2.95.1		F12EBCDIC	actual amount, transaction
2.95.2		F12EBCDIC	unused
2.95.3		F9EBCDIC	unused
2.95.4		F9EBCDIC	unused
2.96		F16HEX		Message Security Code
2.97		F17EBCDIC	Amount, Net Settlement
2.99		B11BCD		Settlement Institution Identification Code
2.100		B11BCD		Receiving Institution Identification Code
2.101		B17EBCDIC	File Name
2.102		B28EBCDIC	Account Identification 1
2.103		B28EBCDIC	Account Identification 2
2.104		B255TLV1BCD	Transaction-Specific Data (Usage 2)
2.104.*		BB252TLV1BCD	dataset ID
2.104.*.*	B250EBCDIC	data
2.104		B100SF		Transaction Description (Usage 1)
2.104.0		F1EBCDIC	billing descriptor
2.104.1		U99EBCDIC	transaction description data
2.105		F32HEX		Double-Length DES Key (Triple DES)
2.115		B24EBCDIC	Additional Trace Data
2.116		B255TLV1BCD	Card Issuer Reference Data
2.116.*		BB252TLV1BCD	dataset ID
2.116.*.*	B250EBCDIC	data
2.117		B255SF		National Use
2.117.1		F3EBCDIC	country code
2.117.2		U252EBCDIC	data
2.118		B255SF		Intra-Country Data
2.118.1		F3EBCDIC	country code
2.118.2		U252EBCDIC	data
2.119		B255SF		Settlement Service Data
2.119.1		F3EBCDIC	country code
2.119.2		U252EBCDIC	data
2.120		B8HEX		Original Message Type ID
2.121		B11EBCDIC	Issuing Institution Identification Code
2.123		B255TLV1BCD	Verification Data (TLV format)
2.123.*		BB252TLV1BCD	dataset ID
2.123.*.*	B250EBCDIC	Verification Data TLV element
2.123		B30SF		Verification Data (fixed format)
2.123.0		F9EBCDIC	postal code
2.123.1		U20EBCDIC	cardholder street address
2.125		B255EBCDIC	Supporting Information
2.126		B255SF		Visa Private-Use Fields
2.126.0		F64BITMAP	Field126 Bitmap
2.126.6		F17SF		Cardholder Certificate Serial Number
2.126.6.0	F2HEX		number of significant digits
2.126.6.1	F32HEX		cardholder certificate serial number
2.126.7		F17SF		Merchant Certificate Serial Number
2.126.7.0	F2HEX		number of significatn digits
2.126.7.1	F32HEX		merchant certificate serial number
2.126.8		F40HEX		Transaction ID (XID)
2.126.9		F20SF		TransStain/CAVV Data
2.126.9.1	F2BCD		3-D Secure Authentication Results Code
2.126.9.2	F2BCD		Second Factor Authentication Code
2.126.9.3	F2BCD		CAVV Key Indicator
2.126.9.4	F3BCD		CAVV Value
2.126.9.5	F4BCD		CAVV Unpredictable Number
2.126.9.6	F16BCD		Authentication Tracking Number
2.126.9.7 	F2BCDSF		Version and Authentication Action
2.126.9.7.0	F1ASC		Version
2.126.9.7.1	F1ASC		Authentication Value
2.126.9.8	F8HEX		IP Address in Hex Format
2.126.10	F6SF		CVV2 Authorization Request Data
2.126.10.1	F1EBCDIC	Presence Indicator
2.126.10.2	F1EBCDIC	Response Type
2.126.10.3	F4EBCDIC	CVV2 Value
2.126.12	F24BITSTR	Service Indicators
2.126.13	F1EBCDIC	POS Environment
2.126.14	F1EBCDIC	Payment Guarantee Option
2.126.15	F1EBCDIC	MC UCAF Collection Indicator
2.126.16	B32EBCDIC	MC UCAF Field
2.126.18	B11SF		Agent Unique Account Result
2.126.18.1	F5EBCDIC
2.126.18.2	F48BITSTR
2.126.19	F1EBCDIC	Dynamic Currency Conversion Indicator
2.127		B255EBCDIC	File Record(s): Action and Data
2.130		F24BITSTR	Terminal Capability Profile
2.131		F40BITSTR	Terminal Verification Results (TVR)
2.132		F8HEX		Unpredictable Number
2.133		F8EBCDIC	Terminal Serial Number
2.134		B510HEX		Visa Discretionary Data
2.135		B30HEX		Issuer Discretionary Data
2.136		F16HEX		Cryptogram
2.137		F4HEX		Application Transaction Counter
2.138		F16BITSTR	Application Interchange Profile
2.139		F10SF		ARPC Response Cryptogram and Code
2.139.1		F16HEX		ARPC Cryptogram
2.139.2		F2EBCDIC	ARPC response code
2.140		B255SF		Issuer Authentication Data
2.140.1		F32HEX		IAD
2.140.2		U478HEX		Reserved
2.142		B510HEX		Issuer Script
2.143		B40HEX		Issuer Script Result
2.144		F2BCD		Cryptogram Transaction Type
2.145		F3BCD		Terminal Country Code
2.146		F6BCD		Terminal Transaction Date
2.147		F12BCD		Cryptogram Amount
2.148		F3BCD		Cryptogram Currency Code
2.149		F12BCD		Cryptogram Cashback Amount
2.152		F16HEX		Secondary PIN Block

#message 	U800SF		Visa Message (with header, no length)
#1		R1		Visa Header
#2		R2		Visa Message
