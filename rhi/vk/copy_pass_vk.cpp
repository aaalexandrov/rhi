#include "copy_pass_vk.h"
#include "rhi_vk.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("copy_pass_vk", [] {
	TypeInfo::Register<CopyPassVk>().Name("CopyPassVk")
		.Base<CopyPass>()
		.Metadata(RhiOwned::s_rhiTagType, TypeInfo::Get<RhiVk>());
});


bool CopyPassVk::Prepare(Submission *sub)
{
	vk::CommandBuffer cmds = _recorder.BeginCmds(_name);
	if (!cmds)
		return false;



	if (!_recorder.EndCmds(cmds))
		return false;

	return true;
}

bool CopyPassVk::Execute(Submission *sub)
{
	return _recorder.Execute(sub);
}

}