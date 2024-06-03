package org.MicroREST

import androidx.lifecycle.ViewModel
import dagger.hilt.android.lifecycle.HiltViewModel
import es.wenea.tpvapp.business.NativeApiEndpoint
import es.wenea.tpvapp.business.NativeServer
import es.wenea.tpvapp.domain.Configuration
import javax.inject.Inject

@HiltViewModel
class ServerViewModel @Inject constructor(): ViewModel() {

    init {
        System.loadLibrary("NativeServer");
        NativeServer.setAuthorizationToken(Configuration.NATIVE_SERVER_TOKEN)
        NativeServer.enableTest()
    }

    fun start(){
        NativeServer.startServer(Configuration.NATIVE_SERVER_PORT)
    }

    fun addApiEndpoints(api: List<NativeApiEndpoint>){
        api.forEach { NativeServer.registerHandler(it.endpoint, it.api) }
    }

}
