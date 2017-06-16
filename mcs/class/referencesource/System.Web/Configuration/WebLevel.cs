//------------------------------------------------------------------------------
// <copyright file="WebLevel.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Web.Configuration {

    // If a path is null, we need this to tell the difference between machine.config
    // or root web.config.
    enum WebLevel {
        Machine = 1,
        Path = 2
    }
}
