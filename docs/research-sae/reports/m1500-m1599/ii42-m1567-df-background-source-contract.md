# M1567 DF-Bounded Background Source Contract

## Objective

M1566 proved that full exact lexical postings plus semantic routes have enough
dense capacity, but common-term lists require 2.55 corpus reads per query.
Term15 is cheap but removes useful low-DF membership. M1567 tests whether a
cost-derived non-indexed background channel separates these effects.

This is one qrels-free source construction, not a DF sweep or learned gate.

## Frozen Construction

- reuse the exact M1565 route basis and route1000 candidates;
- build the same full lexical index as M1566 (`k1=0.82`, `b=0.68`);
- reserve 5% posting reads for semantic routes;
- reserve 25% posting reads for at most 32 lexical query terms;
- index every exact term with corpus DF ratio at most:

```text
max_df_ratio = 0.25 / 32 = 0.0078125
```

- treat higher-DF terms as non-indexed background;
- add at most 256 unique lexical documents to route1000;
- compare BM25 impact order with the same qrels-free dense-top256 oracle used
  in M1566;
- persist all candidates before loading qrels.

Do not tune the cap, term count, route budget, lexical reserve, BM25 parameters,
or score after observing the result.

## Gate

Integrity requires exact M1565 route1000 parity, at most 1,256 combined
candidates, and mean total posting reads at most 30% of corpus size.

The deployable source requires O@10 `>=0.99`, O@100 `>=0.95`, O@256 `>=0.90`,
Recall@100 at least 98% of exact dense, and NDCG/MAP/MRR each at least 99.5%.
The oracle requires O@100 `>=0.98` and O@256 `>=0.95`.

## Decisions

- Deployable pass: authorize protected-tail replay from this unified source.
- Oracle-only pass: authorize one impact/selector distillation stage on this
  fixed source.
- Oracle failure: stop whole-term DF filtering. A later source must alter
  document-level term admission rather than search another global DF cap.
