package org.MicroREST

import androidx.lifecycle.ViewModel
import dagger.hilt.android.lifecycle.HiltViewModel
import javax.inject.Inject

@HiltViewModel
class ServerViewModel @Inject constructor(): ViewModel() {

    init {
        System.loadLibrary("NativeServer");
        NativeServer.enableTest()
    }

    fun start(){
        NativeServer.startServer(8080)
    }

    fun addApiEndpoints(api: List<NativeApiEndpoint>){
        api.forEach { NativeServer.registerHandler(it.endpoint, it.api) }
    }

}
