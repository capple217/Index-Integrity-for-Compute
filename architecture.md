1. Ingest (all records whose time falls in step t)

2. Normalize ($/GPU-hour on the baseline contract lik rental type, country, cpu platform, gpu form)

3. Panel (collapse each provider's records to one price [its median]; mark missing providers)

4. Consensus (over providers that are present **_and_** not held)

5. Residuals (provider j's price minus what the rest of the market implies it should be)

6. Detectors (each provider's CUSUM pair takes its Residual; a missing provider's detector does not update)

7. Classify (the alarms: one or a few [provider issue], or a broad shift [market event])

8.

---

Ingest:

- Reads CSV or JSON snapshots into Quotes
- Maps provider names to dense ids once
- Rejects malformed rows and logs them; never crashes on bad input
