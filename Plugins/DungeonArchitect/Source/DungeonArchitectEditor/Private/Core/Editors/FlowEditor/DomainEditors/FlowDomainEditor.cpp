//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/FlowEditor/DomainEditors/FlowDomainEditor.h"

#include "Frameworks/Flow/Domains/FlowDomain.h"

void IFlowDomainEditor::Initialize(const FDomainEdInitSettings& InSettings) {
    Domain = CreateDomain();
    InitializeImpl(InSettings);
}

FName IFlowDomainEditor::GetDomainID() const {
    check(Domain.IsValid());
    return Domain->GetDomainID();
}

