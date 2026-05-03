/**
 * This file is part of XER, Network open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#include "content/network/cert/cert_transparency.h"

namespace network::cert {

// static
bool CertTransparency::ParseSCTs(const std::vector<uint8_t>& /*extension_data*/,
                                 std::vector<SignedCertTimestamp>* scts) {
    // Placeholder: a full implementation would parse RFC 6962 binary format.
    scts->clear();
    return false;
}

// static
bool CertTransparency::PolicySatisfied(
    const std::vector<SignedCertTimestamp>& scts,
    int min_scts) {
    return static_cast<int>(scts.size()) >= min_scts;
}

}  // namespace network::cert
