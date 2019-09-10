package edu.ufl.cise.os.p2;

import android.graphics.Color;
import android.os.Bundle;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.Spinner;
import android.widget.TextView;
import android.app.Activity;
import android.widget.Toast;
import java.util.ArrayList;
import java.util.List;

public class P2Activity extends Activity {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    int id = 0;
    final int MAX_ALLOCATION_SIZE = 800;

    private void setFragText(int num){
        TextView frag = this.findViewById(R.id.frag_view);
        frag.setText(String.valueOf(num));
    }

    private void setFreeText(int num){
        TextView free = this.findViewById(R.id.free_view);
        free.setText(String.valueOf(num));
    }

    private void setUseText(int num){
        TextView use = this.findViewById(R.id.use_view);
        use.setText(String.valueOf(num));
    }

    private void updateTextViews(){
        int frag_size = getFragSize();
        setFragText(frag_size);

        int free_size = getFreeSize();
        setFreeText(free_size);

        int use_size = getUseSize();
        setUseText(use_size);
    }

    private int getAllocSize(){
        EditText allocView = this.findViewById(R.id.alloc_view);
        String val = allocView.getText().toString();
        if (val.equals("")){
            val = "4";
        }
        return Integer.parseInt(val);
    }

    private void initAlgorithms(){
        List<String> spinnerOptions = new ArrayList<>();
        spinnerOptions.add("Best");
        spinnerOptions.add("Worst");

        Spinner algs = this.findViewById(R.id.spinner);
        ArrayAdapter<String> adapter = new ArrayAdapter<String>(this,android.R.layout.simple_spinner_item,spinnerOptions);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        algs.setAdapter(adapter);
    }

    private int interpretAlgorithms(){
        Spinner algs = this.findViewById(R.id.spinner);
        String alg = algs.getSelectedItem().toString();
        if (alg.equals("Best")){
            return 1;
        }
        else{
            return 2;
        }
    }

    private void addMemory(LinearLayout linearLayout){
        LinearLayout linear1 = new LinearLayout(this);
        linear1.setOrientation(LinearLayout.HORIZONTAL);
        linearLayout.addView(linear1);

        int allocation_size = getAllocSize();
        int selected_algorithm = interpretAlgorithms();
        setAlgorithm(selected_algorithm);
        final String addr = allocateMemory(allocation_size);

        if (addr.equals("RIP")){
            Toast.makeText(getApplicationContext(), "Memory requested over the allocated limit!", Toast.LENGTH_SHORT).show();
        }
        else{
            Button b;
            b = new Button(this);
            b.setText("Memory Chunk "+addr);
            b.setId(id);
            b.setTextSize(30);

            b.setTextColor(Color.parseColor("#f51e16"));
            b.setPadding(0,0,0,0);

            LinearLayout.LayoutParams ll = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,LinearLayout.LayoutParams.MATCH_PARENT);
            ll.gravity = Gravity.CENTER;
            b.setLayoutParams(ll);
            linear1.addView(b);
            id++;
            Toast.makeText(getApplicationContext(), "Memory Chunk "+(id-1)+" added!", Toast.LENGTH_SHORT).show();

            b.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    freeMemory(addr);
                    updateTextViews();
                    Toast.makeText(getApplicationContext(), "Memory "+addr+" deleted!", Toast.LENGTH_SHORT).show();
                    ViewGroup parentView = (ViewGroup) v.getParent();
                    parentView.removeView(v); // Removes button from layout
                }
            });
        }
    }

    private void initSubmitBtn(){
        Button submit = (Button) findViewById(R.id.submitButton);
        submit.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                shutdown();
                LinearLayout linearlayout = findViewById(R.id.button_layout);
                id = 0;
                linearlayout.removeAllViews();
                updateTextViews();
                Toast.makeText(getApplicationContext(), "All Memory Cleared!", Toast.LENGTH_SHORT).show();
            }
        });
    }

    private void initAddBtn(){
        Button add = (Button) findViewById(R.id.addButton);
        add.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                addMemory((LinearLayout) findViewById(R.id.button_layout));
                updateTextViews();
            }
        });
    }

    private void startMemoryManager(){
        initMemoryManager(MAX_ALLOCATION_SIZE);
        updateTextViews();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_p2);

        initAlgorithms();

        initSubmitBtn();

        initAddBtn();

        startMemoryManager();
    }

    @Override
    protected void onStop(){
        super.onStop();
        writeLogs();
        deleteMemoryManager();
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     *
     * For details on functions, see description in native-calls.cpp
     */

    public native String stringFromJNI();

    public native void initMemoryManager(int maxAllocationSize);

    public native void deleteMemoryManager();

    public native int getFragSize();

    public native int getFreeSize();

    public native int getUseSize();

    public native String allocateMemory(int size);

    public native void freeMemory(String addr);

    public native void shutdown();

    public native void setAlgorithm(int algorithm);

    public native void writeLogs();

}
