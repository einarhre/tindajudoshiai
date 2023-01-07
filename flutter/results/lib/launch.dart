import 'package:flutter/material.dart';
import 'package:results/utils.dart';
import 'package:url_launcher/url_launcher_string.dart';

class WebPage extends StatefulWidget {
  final String filename;

  WebPage({required this.filename, Key? key}) : super(key: key);

  @override
  State<WebPage> createState() => _WebPageState();
}

class _WebPageState extends State<WebPage> {
  void _viewFile() async {
    String url = '$hostUrl/${widget.filename}';

    try {
      await launchUrlString(url);
    } catch (err) {
      debugPrint('Something went wrong');
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Kindacode.com'),
      ),
      body: Center(
        child: ElevatedButton(
          onPressed: _viewFile,
          child: const Text('View PDF'),
        ),
      ),
    );
  }
}

Future<void> launchWebPage(String filename) async {
  String url = '$hostUrl/${filename}';

  try {
    await launchUrlString(url);
  } catch (err) {
    debugPrint('Something went wrong');
  }
}