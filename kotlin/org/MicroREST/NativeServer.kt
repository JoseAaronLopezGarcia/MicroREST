package org.MicroREST

import java.lang.Exception

data class NativeApiEndpoint(
    val endpoint: String,
    val api: NativeServerApi
)

abstract class NativeServerApi {
    private var code: Int = 200;
    private var response: String = "";

    fun handle(method: String, body: String){
        try {
            when (method) {
                "GET" -> this.get(body)
                "POST" -> this.post(body)
                "PUT" -> this.put(body)
                "DELETE" -> this.delete(body)
                else -> setResult(400, "")
            }
        }
        catch (e: Exception){
            setResult(500, "")
        }
    }

    open fun get(body: String){ setResult(501, "") }
    open fun post(body: String){ setResult(501, "") }
    open fun put(body: String){ setResult(501, "") }
    open fun delete(body: String){ setResult(501, "") }

    fun setResult(code: Int, response: String){
        this.code = code
        this.response = response
    }

    fun getCode(): Int {
        return code
    }

    fun getResponse(): String {
        return response
    }
}

object NativeServer {

    @JvmStatic
    external fun setAuthorizationToken(token: String)

    @JvmStatic
    external fun enableTest()

    @JvmStatic
    external fun registerHandler(path: String, handler: NativeServerApi)

    @JvmStatic
    external fun startServer(port: Int)

}
