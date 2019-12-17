//
//  INIParser.swift
//  DolphiniOS
//
//  Created by Tucker Morley on 12/16/19.
//  Copyright © 2019 Dolphin Team. All rights reserved.
//

import Foundation

@objc class DolphinINIParser: NSObject {
    
    @objc public init(contents: String) {
        self.contents = contents;
    }
    
    @objc var contents: String
    
    // MARK: Converter
    @objc public func convert() -> [[String:Any]] {
        let lines = self.contents.components(separatedBy: "\n")
        var sections: [[String:Any]] = []
        var currentSection = 0
        for line in lines {
            switch self.getLineType(for: line) {
            case .header:
                let header = line.replacingOccurrences(of: "[", with: "").replacingOccurrences(of: "]", with: "")
                sections.append(["name": header, "variables": [:],])
                currentSection += 1
                break;
            case .keyValue:
                let keyValue = line.components(separatedBy: "=")
                let key = keyValue[0].trimmingCharacters(in: .whitespacesAndNewlines)
                let value = keyValue[1].trimmingCharacters(in: .whitespacesAndNewlines)
                var variables = sections[currentSection - 1]["variables"] as! [String:String]
                variables[key] = value
                sections[currentSection - 1]["variables"] = variables
                break;
            default:
                break;
            }
        }
        return sections
    }
    
    @objc static func createINI(from dictionary: [[String:Any]]) -> String {
        let iniString: String = dictionary.map { section in
            let variables = section["variables"] as! [String:String]
            return """
            [\(section["name"] ?? "")]
            \(variables.map{ "\($0) = \($1)" }.joined(separator: "\n"))
            """
        }.joined(separator: "\n")
        return iniString
    }
    
    // MARK: Line Type
    @objc enum LineType: NSInteger {
        case unknown
        case comment
        case header
        case keyValue
    }
    
    func getLineType(for line: String) -> LineType {
        
        // Define Regexes
        let headerRegex = try! NSRegularExpression(pattern: "\\[(.*)\\]")
        let commentRegex = try! NSRegularExpression(pattern: "; (.*)")
        let keyValueRegex = try! NSRegularExpression(pattern: "(.*) = (.*)")

        // Find Matches
        let headerMatches = headerRegex.matches(in: line, options: [], range: NSRange(location: 0, length: line.utf16.count))
        let commentMatches = commentRegex.matches(in: line, options: [], range: NSRange(location: 0, length: line.utf16.count))
        let keyValueMatches = keyValueRegex.matches(in: line, options: [], range: NSRange(location: 0, length: line.utf16.count))
        
        // Return enum
        if headerMatches.count > 0 {
            return .header
        } else if keyValueMatches.count > 0 {
            return .keyValue
        } else if commentMatches.count > 0 {
            return .comment
        }
        return .unknown
    }
}

