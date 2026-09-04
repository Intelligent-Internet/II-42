# M1724 Query-Cost-Aware Residual Source Contract

## Objective

M1723 demonstrates a real joint residual-source gain but exposes a precise
quality/cost conflict. Its best quality checkpoint improves O@100/O@256 by
`+0.014720/+0.032445`, while semantic reads reach `0.191940x`. Maximum DF is
only `0.014006`, so stronger document max-DF pressure does not target the
failure.

M1724 adds the missing runtime quantity to the objective:

```text
df_batch[g, k] = mean hard document assignment to key (g, k)
expected_reads(q) = probes * sum(q_assignment[g, k] * df_batch[g, k])
L_query_cost = relu(expected_reads - 0.18)^2
```

Both assignments use straight-through hard forward values and soft backward
gradients. The estimate counts posting-list reads, including repeated document
visits across keys, matching the M1723 cost metric more closely than average
activation or max DF.

## Frozen Comparison

Everything except this loss term remains identical to M1723:

- same 4,000/500 pools and BM25 caches;
- same M1600 initialization;
- same 8x512 namespace, 1 document key/group, 8 query probes/group;
- same 128 candidate rows and 96 teacher documents;
- same dense-control and residual arms;
- same learning rate, batch size, temperature schedule, and hard gates.

The fixed coefficient is `lambda_query_cost=100`. The excess at the M1723
quality optimum is about `0.012`, so its squared weighted penalty is roughly
`0.014`, large enough to affect key choice but small relative to the listwise
loss. There is no coefficient sweep.

The schedule is 1,200 updates. M1723 established that all useful checkpoints
occur by step 700 and later depth is destructive, so repeating 2,000 updates
would add cost without information.

## Gate

Use the unchanged M1723 scale gate:

- trained residual checkpoint;
- O@100 gain `>=0.01`, O@256 gain `>=0.005`;
- residual R@100 gain `>=0.01`, R@256 gain `>=0.005`;
- exact semantic reads `<=0.18x` and max DF `<=0.02`;
- residual variant score at least `0.002` above dense control.

Only a conjunctive pass authorizes 10,000/1,000 replication. A pass must then
replicate under a second seed before native scorer work.

## Stop Rules

- Do not change query probes or read ceiling after failure.
- Do not sweep the query-cost coefficient.
- If quality remains above the M1723 control only by exceeding `0.18x`, record
  the Pareto frontier and stop.
- If expected batch reads fall but exact heldout reads do not, reject the
  surrogate rather than increasing its weight.
- If the cost-aware canary passes, the next stage is scale replication, not
  BM25 alpha, qrels, or reranking.
