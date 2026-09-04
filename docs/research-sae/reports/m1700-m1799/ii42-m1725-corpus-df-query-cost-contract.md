# M1725 Corpus-DF Query-Cost Contract

## Objective

M1724 proves that minibatch document marginals are an inadequate proxy for
the cost paid by query-selected keys. M1725 replaces only that estimator with
a periodically refreshed full-corpus hard DF table.

For the current source checkpoint:

```text
df[g, k] = count(doc assigned to (g, k)) / document_count
expected_reads(q) = sum over the 64 hard query probes of df[g, k]
```

The table is recomputed over all 35,831 training documents every 50 updates.
It is detached during the interval, so cost gradients modify query/source key
compatibility without pretending that the discrete corpus counts are locally
differentiable. Document balance and max-DF losses continue to update the
source and the table then refreshes.

## Frozen Configuration

M1725 retains M1724 exactly:

- 4,000/500 pools and frozen BM25 caches;
- M1600 8x512 initialization;
- one document key and eight query probes per group;
- 128 candidates and 96 teacher documents;
- query cost target `0.18x`, coefficient 100;
- dense-control and residual arms;
- 1,200 updates, batch 16, learning rate `1e-4`;
- unchanged quality, max-DF, read, and control-advantage gates.

No qrels, coefficient sweep, probe sweep, or threshold tuning is allowed.

## Gate And Stop

The canary passes only if residual training produces:

- O@100 gain `>=0.01`, O@256 gain `>=0.005`;
- residual R@100 gain `>=0.01`, R@256 gain `>=0.005`;
- exact heldout reads `<=0.18x`, max DF `<=0.02`;
- a variant score at least `0.002` above the same-seed dense control.

A pass authorizes 10,000/1,000 replication and a second seed. Failure ends
the M1723-M1725 joint residual-source cost-repair family; no further DF proxy,
lambda, probe, depth, or budget variants are authorized.
