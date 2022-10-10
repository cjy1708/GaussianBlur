package com.ziyuanrenjianjinhao.gaussianblur

import android.graphics.Bitmap
import android.graphics.BitmapFactory
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import android.view.ViewGroup
import android.widget.ImageView
import android.widget.TextView
import androidx.core.graphics.get
import com.ziyuanrenjianjinhao.gaussianblur.databinding.ActivityMainBinding
import java.lang.reflect.Array
import kotlin.concurrent.thread
import kotlin.system.measureTimeMillis

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        thread {
            val bitmap = BitmapFactory.decodeResource(resources, R.drawable.pic2)
            val bitmapGaussianBlur =
                Bitmap.createBitmap(bitmap.width, bitmap.height, Bitmap.Config.ARGB_8888)
            var bitmapIntArray = IntArray(bitmap.width * bitmap.height)
            bitmap.getPixels(bitmapIntArray, 0, bitmap.width, 0, 0, bitmap.width, bitmap.height)
            val bitmapSource = intArrayOf(*bitmapIntArray)
            Log.d("CJY", "变化前(599, 599)：${bitmapIntArray[599 * bitmap.width + 599].toString()}")
            Log.d("CJY", "start trace")
            val cost = measureTimeMillis {
                gaussianBlur(bitmapIntArray, bitmap.width, bitmap.height, 3, 0.00000005)
            }

            Log.d("CJY", "end trace")
            Log.d("CJY", "变化后(599, 599)：${ bitmapIntArray[599 * bitmap.width + 599].toString() }")
            Log.d("CJY", "变化前后是否有差异：${bitmapIntArray.contentEquals(bitmapSource).toString()}")

            bitmapGaussianBlur.setPixels(
                bitmapIntArray,
                0,
                bitmap.width,
                0,
                0,
                bitmap.width,
                bitmap.height
            )
            runOnUiThread {
                binding.root.addView(ImageView(this).apply {
                    setImageBitmap(bitmapGaussianBlur)
                })
            }

            Log.d("CJY", cost.toString())
        }
    }

    external fun gaussianBlur(pixels: IntArray, width: Int, height: Int, kernelSize: Int, sigma: Double)

    companion object {
        // Used to load the 'gaussianblur' library on application startup.
        init {
            System.loadLibrary("gaussianblur")
        }
    }
}