# II-42 M1938 Co-Keyed Multi-Channel Posting Contract

## Question

M1937 showed that input-aligned semantic impacts cannot be deleted without
losing Recall. M1938 tests whether lexical and semantic evidence can instead
share physical posting keys while preserving independent score channels.

For a safely mapped term, one physical `(term, document)` entry may contain:

- lexical BM25 impact;
- semantic M1914 impact;
- or both.

Query payloads retain the matching lexical and semantic weights. Score
contribution remains exactly:

```text
lexical_query * lexical_document
  + semantic_scale * semantic_query * semantic_document
```

No cross terms are introduced. This is one physical inverted index with
multi-channel postings, not ANN plus BM25 and not score fusion after retrieval.

## Conservative Mapping

A Granite vocabulary dimension may share a native lexical key only when:

1. standalone decoding yields exactly one lowercase, whitespace-free native
   lexical token;
2. re-encoding that token reproduces the same Granite dimension;
3. at most one preferred word-boundary Granite dimension maps to the lexical
   token.

Ambiguous subwords and collisions remain in the semantic namespace. This
underestimates possible compression but avoids changing score identity.

## Cost Model

Report for disjoint `b1`, co-keyed `b1`, disjoint nested `b1.125`, and co-keyed
nested `b1.125`:

- physical document-key entries;
- separate impact-channel values;
- overlap entries carrying both channels;
- channel-aware query posting touches;
- naive union-list touches as an engineering upper bound.

The channel-aware layout stores lexical-only, semantic-only, and shared
substreams under one term key. A query using one channel need not traverse the
other-only substream. A naive union list does not have that property and is
reported separately.

## Progressive Gate

Run SciDocs first. Authorize Quora and TREC-COVID only when co-keyed nested
`b1.125`:

- has no more physical document-key entries than disjoint `b1`;
- has no more channel-aware mean touches than disjoint `b1`;
- preserves every lexical and semantic impact value by construction;
- uses only collision-free mappings;
- maps at least 1% of active semantic document postings.

A three-row pass authorizes an engine-format prototype and exact score replay.
It does not authorize model training. If the conservative mapping cannot
offset the 12.5% semantic expansion on SciDocs, close this exact co-key route
before changing tokenizer equivalence or payload layout.
