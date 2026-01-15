#include "ETRI_KSL_Excel_DB-v1.4.h"

#include <unordered_set>
#include <unordered_map>

#define MIN_SIMILARITY 1.4

// infer_order 별 가중치 (0이 제일 신뢰)
static const double ORDER_WEIGHT[3] = { 1.0, 0.7, 0.4 };

// 같은 sgIndex가 여러 frame에서 나올 때 보너스
static const double DUP_GAIN = 0.3;

// corpus 단위 기본 스코어 파라미터
static const double BETA = 0.2;  // 서로 다른 matchedCount 증가 보너스
static const double GAMMA = 0.7;  // coverage^GAMMA

// 연속된 gloss 시퀀스 보너스 (폭발적으로 증가시키는용)
static const double SEQ_GAIN = 0.6;   // 값 키우면 연속 시퀀스 영향 ↑

typedef struct {
	int sgIndex;
	int cpIndex;
	int frame_index;
	int infer_order;
	double prob;
} GlossInfered;


#pragma once
class InferSentence
{
public:
	void BuildCorpusGlossWeight(std::vector<_typeCorpusVideoDB>& CorpusVideoDB);
	_typeCorpusVideoDB InferCorpus(std::vector<GlossInfered> inferList, std::vector<_typeCorpusVideoDB>& CorpusVideoDB);

};

