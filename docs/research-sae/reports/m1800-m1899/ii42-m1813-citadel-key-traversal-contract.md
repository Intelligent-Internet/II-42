# M1813 CITADEL Key-Traversal Contract

## Trigger

M1812 preserves qrels quality but fails O@100 because the current bounded
engine skips an entire posting list when that list does not fit the remaining
entry budget. SciFact's largest list has 9,132 token entries for a 5,183
document corpus; NFCorpus reaches 10,298 entries for 3,633 documents.

## Fixed Surface

Reuse the frozen M1812 caches and the fixed q1/d5/prune0.5/int8-32 policy. No
model encoding, training, qrels-driven policy selection, or dataset-specific
parameter is allowed.

Compare:

1. exhaustive float learned-route truth;
2. full int8 traversal of every selected query key;
3. the M1812 whole-list `1x documents` cap;
4. a `1x documents` impact-ordered traversal that stores each key list in
   descending document route-weight order and consumes a prefix when needed.

The impact order is query-independent and qrels-free. It is a native posting
layout, not a reranker.

## Gate

Full traversal authorizes the representation if both datasets satisfy O@100
at least `0.95`, all qrels metrics remain within `0.01` of float exact, average
payload is at most `1 MiB/query`, and the index remains at most
`10 KiB/document`.

If full traversal exceeds cost, impact ordering may pass with O@100 at least
`0.90`, qrels metrics within `0.02`, and the same cost envelope. Otherwise the
vector-payload posting route stops at the current product budget.
