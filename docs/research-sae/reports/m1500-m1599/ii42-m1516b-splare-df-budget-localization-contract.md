# M1516B SPLARE DF-Budget Mechanism Localization Contract

## Why this follow-up exists

M1516 is a valid negative gate: none of its predeclared configurations
qualified. It nevertheless changed DF cap and per-row budget together between
the most informative `DF20/D128/Q32` and `DF10/D64/Q16` points. The observed
quality loss therefore cannot yet be assigned to corpus-frequency filtering
or to row truncation.

M1516B is one post-hoc mechanism-localization audit. It cannot promote a model
or authorize training on the same three evaluation datasets.

## Fixed localization grid

Hold document/query budgets at D128/Q32 and test only:

- DF 15%, no IDF;
- DF 15%, one-sided IDF;
- DF 10%, no IDF;
- DF 10%, one-sided IDF.

The M1510 D400/Q40 row remains the parity control. The same global settings
run on complete NFCorpus, SciFact, and FiQA. No further cap interpolation is
allowed after this audit.

## Interpretation

- If no point clears the original M1516 quality/cost gate, stop this
  source-construction route.
- If a point clears the gate, it is only a candidate. It must reproduce on a
  newly encoded, untouched corpus before any M1517 objective training is
  authorized.
- Per-dataset caps, qrels-derived atom filters, and post-hoc fallbacks remain
  prohibited.
