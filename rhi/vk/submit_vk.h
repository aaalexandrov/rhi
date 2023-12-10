#pragma once

#include "base_vk.h"
#include "../submit.h"

namespace rhi {

struct SubmissionVk : public Submission {


	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<SubmissionVk>(); }

};

}