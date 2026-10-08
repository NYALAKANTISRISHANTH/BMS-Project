# Bug Log

| ID | Bug | Cause | Fix | Status |
|---|---|---|---|---|
| BUG-001 | Stable equal cell readings could be reported as a fault | Incorrect frozen-value detection treated repeated valid readings as an error | Removed the false frozen-reading fault condition | Fixed |
| BUG-002 | Initial implementation supported only four cell inputs | Fixed four-pin ADC mapping | Changed cell acquisition to a 16:1 multiplexer architecture | Fixed |
| BUG-003 | Risk/health indication could appear unexpected | Risk score is derived from multiple normalized factors and health bands | Verified the calculation and documented the health mapping | Fixed |

## Verification Note
Only bugs actually observed during development are listed here. New issues should be added with cause, corrective action and status.
